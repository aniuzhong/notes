// Wayland + libdecor window; Vulkan swapchain triangle; VMA for vertex buffer upload.

#include <libdecor.h>
#include <wayland-client.h>

#ifndef VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR 1
#endif
#include <vulkan/vulkan.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

#ifndef WV_SHADER_SPV_DIR
#define WV_SHADER_SPV_DIR "."
#endif

namespace {

// ─── Wayland / libdecor ─────────────────────────────────────────────────────
wl_display*            wl_dpy       = nullptr;
wl_compositor*         compositor   = nullptr;
wl_surface*            surface      = nullptr;
libdecor*                decor_ctx    = nullptr;
libdecor_frame*          decor_frame  = nullptr;

int      content_w = 800, content_h = 600;
bool     configured        = false;
bool     closing           = false;
bool     swapchain_dirty   = true;

// ─── Vulkan + VMA (swapchain resources recreated on resize) ───────────────────
vk::Instance               instance;
vk::SurfaceKHR             vk_surface;
vk::PhysicalDevice         physical_device;
vk::Device                 device;
vk::Queue                  graphics_queue;
uint32_t                   queue_family = 0;

vk::SwapchainKHR           swapchain;
vk::Format                 swap_format = vk::Format::eUndefined;
vk::Extent2D               swap_extent{0, 0};
std::vector<vk::Image>     swap_images;
std::vector<vk::ImageView> swap_image_views;
std::vector<vk::Framebuffer> framebuffers;

vk::RenderPass     render_pass;
vk::PipelineLayout pipeline_layout;
vk::Pipeline       graphics_pipeline;

vk::CommandPool                command_pool;
std::vector<vk::CommandBuffer> command_buffers;

vk::Semaphore image_available;
vk::Semaphore render_finished;
vk::Fence     in_flight;

VmaAllocator           vma_allocator   = VK_NULL_HANDLE;
VkBuffer               vertex_buffer  = VK_NULL_HANDLE;
VmaAllocation          vertex_alloc    = VK_NULL_HANDLE;

[[noreturn]] void fatal(const char* msg) {
    fprintf(stderr, "%s\n", msg);
    std::exit(EXIT_FAILURE);
}

std::vector<std::uint32_t> read_spv(const fs::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
        fatal("failed to open SPIR-V file (check GLSL compile / WV_SHADER_SPV_DIR)");
    const auto size = static_cast<std::size_t>(f.tellg());
    if (size % 4 != 0 || size == 0)
        fatal("invalid SPIR-V size");
    std::vector<std::uint32_t> buf(size / 4);
    f.seekg(0);
    f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(size));
    return buf;
}

vk::ShaderModule make_shader_module(const std::vector<std::uint32_t>& code) {
    vk::ShaderModuleCreateInfo ci;
    ci.setCode(code);
    return device.createShaderModule(ci);
}

void registry_handler(void*, wl_registry* registry, std::uint32_t name,
                     const char* iface, std::uint32_t) {
    if (std::strcmp(iface, wl_compositor_interface.name) == 0)
        compositor = static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, 4));
}

void registry_remover(void*, wl_registry*, std::uint32_t) {}

const wl_registry_listener registry_listener = {registry_handler, registry_remover};

void libdecor_on_error(libdecor*, libdecor_error, const char* message) {
    fprintf(stderr, "libdecor error: %s\n", message ? message : "(null)");
}

void decor_frame_configure(libdecor_frame* frame, libdecor_configuration* cfg, void*) {
    int w = content_w, h = content_h;
    if (libdecor_configuration_get_content_size(cfg, frame, &w, &h)) {
        content_w = w;
        content_h = h;
    }
    libdecor_state* state = libdecor_state_new(content_w, content_h);
    libdecor_frame_commit(frame, state, cfg);
    libdecor_state_free(state);
    swapchain_dirty = true;
    configured      = true;
}

void decor_frame_close(libdecor_frame*, void*) { closing = true; }

void decor_frame_commit_cb(libdecor_frame*, void*) { wl_surface_commit(surface); }

void decor_frame_dismiss_popup(libdecor_frame*, const char*, void*) {}

libdecor_interface       libdecor_iface = {};
libdecor_frame_interface frame_iface    = {};

void pump_until_configured() {
    int guard = 0;
    while (!configured && wl_display_get_error(wl_dpy) == 0) {
        if (++guard > 100000) {
            fprintf(stderr,
                    "Timeout: no libdecor configure (install libdecor plugin, e.g. "
                    "libdecor-0-plugin-1-cairo)\n");
            std::exit(EXIT_FAILURE);
        }
        wl_display_dispatch(wl_dpy);
        libdecor_dispatch(decor_ctx, 0);
    }
}

// Queue family with graphics + present to surface
std::optional<std::uint32_t> find_queue_family(vk::PhysicalDevice pd, vk::SurfaceKHR surf) {
    auto props = pd.getQueueFamilyProperties();
    for (std::uint32_t i = 0; i < props.size(); ++i) {
        if (!(props[i].queueFlags & vk::QueueFlagBits::eGraphics))
            continue;
        vk::Bool32 present_ok = false;
        if (pd.getSurfaceSupportKHR(i, surf, &present_ok) != vk::Result::eSuccess)
            continue;
        if (present_ok)
            return i;
    }
    return std::nullopt;
}

vk::SurfaceFormatKHR pick_surface_format(const std::vector<vk::SurfaceFormatKHR>& formats) {
    for (const auto& f : formats) {
        if (f.format == vk::Format::eB8G8R8A8Srgb &&
            f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return f;
    }
    return formats.front();
}

vk::PresentModeKHR pick_present_mode(const std::vector<vk::PresentModeKHR>& modes) {
    for (auto m : modes) {
        if (m == vk::PresentModeKHR::eMailbox)
            return m;
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D pick_extent(const vk::SurfaceCapabilitiesKHR& caps, int w, int h) {
    if (caps.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
        return caps.currentExtent;
    vk::Extent2D e{static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h)};
    e.width  = std::clamp(e.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    e.height = std::clamp(e.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return e;
}

vk::CompositeAlphaFlagBitsKHR pick_composite_alpha(vk::CompositeAlphaFlagsKHR supported) {
    const vk::CompositeAlphaFlagBitsKHR candidates[] = {
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
        vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
        vk::CompositeAlphaFlagBitsKHR::eInherit,
    };
    for (auto c : candidates) {
        if (supported & c)
            return c;
    }
    return vk::CompositeAlphaFlagBitsKHR::eInherit;
}

void destroy_swapchain_dependent() {
    if (!device)
        return;
    device.waitIdle();
    if (graphics_pipeline) {
        device.destroyPipeline(graphics_pipeline);
        graphics_pipeline = nullptr;
    }
    if (pipeline_layout) {
        device.destroyPipelineLayout(pipeline_layout);
        pipeline_layout = nullptr;
    }
    if (render_pass) {
        device.destroyRenderPass(render_pass);
        render_pass = nullptr;
    }
    for (auto fb : framebuffers) {
        if (fb)
            device.destroyFramebuffer(fb);
    }
    framebuffers.clear();
    for (auto v : swap_image_views) {
        if (v)
            device.destroyImageView(v);
    }
    swap_image_views.clear();
    swap_images.clear();
    // Swapchain itself is destroyed by vkCreateSwapchainKHR(oldSwapchain) or destroy_swapchain_all().
}

void destroy_swapchain_all() {
    if (!device)
        return;
    device.waitIdle();
    destroy_swapchain_dependent();
    if (swapchain) {
        device.destroySwapchainKHR(swapchain);
        swapchain = nullptr;
    }
    swap_extent = vk::Extent2D{0, 0};
}

// Alias for full teardown at program exit.
void destroy_swapchain_and_graphics() { destroy_swapchain_all(); }

void create_render_pass_and_pipeline(const std::vector<std::uint32_t>& vert_spv,
                                    const std::vector<std::uint32_t>& frag_spv) {
    vk::AttachmentDescription color_att;
    color_att.format         = swap_format;
    color_att.samples        = vk::SampleCountFlagBits::e1;
    color_att.loadOp         = vk::AttachmentLoadOp::eClear;
    color_att.storeOp        = vk::AttachmentStoreOp::eStore;
    color_att.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    color_att.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    color_att.initialLayout  = vk::ImageLayout::eUndefined;
    color_att.finalLayout    = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference color_ref;
    color_ref.attachment = 0;
    color_ref.layout     = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint    = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &color_ref;

    vk::SubpassDependency dep;
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dep.dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dep.srcAccessMask = {};
    dep.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo rpci;
    rpci.attachmentCount = 1;
    rpci.pAttachments    = &color_att;
    rpci.subpassCount    = 1;
    rpci.pSubpasses      = &subpass;
    rpci.dependencyCount = 1;
    rpci.pDependencies   = &dep;
    render_pass          = device.createRenderPass(rpci);

    vk::PushConstantRange pcr;
    pcr.stageFlags = vk::ShaderStageFlagBits::eFragment;
    pcr.offset     = 0;
    pcr.size       = sizeof(float);

    vk::PipelineLayoutCreateInfo plci;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges    = &pcr;
    pipeline_layout             = device.createPipelineLayout(plci);

    auto vs_mod = make_shader_module(vert_spv);
    auto fs_mod = make_shader_module(frag_spv);

    vk::PipelineShaderStageCreateInfo vs_stage;
    vs_stage.stage  = vk::ShaderStageFlagBits::eVertex;
    vs_stage.module = vs_mod;
    vs_stage.pName  = "main";

    vk::PipelineShaderStageCreateInfo fs_stage;
    fs_stage.stage  = vk::ShaderStageFlagBits::eFragment;
    fs_stage.module = fs_mod;
    fs_stage.pName  = "main";

    vk::PipelineShaderStageCreateInfo stages[] = {vs_stage, fs_stage};

    vk::VertexInputBindingDescription vbind;
    vbind.binding   = 0;
    vbind.stride    = sizeof(float) * 3;
    vbind.inputRate = vk::VertexInputRate::eVertex;

    vk::VertexInputAttributeDescription vattr;
    vattr.location = 0;
    vattr.binding  = 0;
    vattr.format   = vk::Format::eR32G32B32Sfloat;
    vattr.offset   = 0;

    vk::PipelineVertexInputStateCreateInfo vi;
    vi.vertexBindingDescriptionCount   = 1;
    vi.pVertexBindingDescriptions      = &vbind;
    vi.vertexAttributeDescriptionCount = 1;
    vi.pVertexAttributeDescriptions    = &vattr;

    vk::PipelineInputAssemblyStateCreateInfo ia;
    ia.topology = vk::PrimitiveTopology::eTriangleList;

    vk::PipelineViewportStateCreateInfo vp;
    vp.viewportCount = 1;
    vp.scissorCount  = 1;

    vk::PipelineRasterizationStateCreateInfo rs;
    rs.polygonMode = vk::PolygonMode::eFill;
    rs.cullMode    = vk::CullModeFlagBits::eNone;
    rs.frontFace   = vk::FrontFace::eCounterClockwise;
    rs.lineWidth   = 1.f;

    vk::PipelineMultisampleStateCreateInfo ms;
    ms.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState cba;
    cba.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo blend;
    blend.attachmentCount = 1;
    blend.pAttachments    = &cba;

    std::vector<vk::DynamicState> dyn = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dyn_ci;
    dyn_ci.dynamicStateCount = static_cast<std::uint32_t>(dyn.size());
    dyn_ci.pDynamicStates    = dyn.data();

    vk::GraphicsPipelineCreateInfo gpci;
    gpci.stageCount          = 2;
    gpci.pStages             = stages;
    gpci.pVertexInputState   = &vi;
    gpci.pInputAssemblyState = &ia;
    gpci.pViewportState      = &vp;
    gpci.pRasterizationState = &rs;
    gpci.pMultisampleState   = &ms;
    gpci.pColorBlendState    = &blend;
    gpci.pDynamicState       = &dyn_ci;
    gpci.layout              = pipeline_layout;
    gpci.renderPass          = render_pass;

    graphics_pipeline =
        device.createGraphicsPipeline(vk::PipelineCache{}, gpci).value;

    device.destroyShaderModule(fs_mod);
    device.destroyShaderModule(vs_mod);
}

void create_swapchain_and_framebuffers(const std::vector<std::uint32_t>& vert_spv,
                                      const std::vector<std::uint32_t>& frag_spv) {
    if (content_w <= 0 || content_h <= 0)
        return;

    if (!command_buffers.empty()) {
        device.freeCommandBuffers(command_pool, command_buffers);
        command_buffers.clear();
    }

    auto caps         = physical_device.getSurfaceCapabilitiesKHR(vk_surface);
    auto formats      = physical_device.getSurfaceFormatsKHR(vk_surface);
    auto presentModes = physical_device.getSurfacePresentModesKHR(vk_surface);

    if (formats.empty() || presentModes.empty()) {
        fprintf(stderr, "No surface formats or present modes (surface may not be ready).\n");
        return;
    }

    vk::SurfaceFormatKHR surface_format = pick_surface_format(formats);
    swap_format                         = surface_format.format;

    vk::PresentModeKHR present_mode = pick_present_mode(presentModes);
    swap_extent                      = pick_extent(caps, content_w, content_h);

    if (swap_extent.width == 0 || swap_extent.height == 0) {
        fprintf(stderr, "Swapchain extent is zero; skipping create.\n");
        return;
    }

    std::uint32_t image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
        image_count = caps.maxImageCount;

    vk::SwapchainKHR old_swapchain = swapchain;
    destroy_swapchain_dependent();

    vk::SwapchainCreateInfoKHR sci;
    sci.surface                 = vk_surface;
    sci.minImageCount           = image_count;
    sci.imageFormat             = swap_format;
    sci.imageColorSpace         = surface_format.colorSpace;
    sci.imageExtent             = swap_extent;
    sci.imageArrayLayers        = 1;
    sci.imageUsage              = vk::ImageUsageFlagBits::eColorAttachment;
    sci.imageSharingMode        = vk::SharingMode::eExclusive;
    sci.queueFamilyIndexCount   = 0;
    sci.pQueueFamilyIndices     = nullptr;
    sci.preTransform            = caps.currentTransform;
    sci.compositeAlpha          = pick_composite_alpha(caps.supportedCompositeAlpha);
    sci.presentMode             = present_mode;
    sci.clipped                 = VK_TRUE;
    sci.oldSwapchain            = old_swapchain;

    swapchain = device.createSwapchainKHR(sci);
    swap_images = device.getSwapchainImagesKHR(swapchain);

    for (auto img : swap_images) {
        vk::ImageViewCreateInfo ivci;
        ivci.image                           = img;
        ivci.viewType                        = vk::ImageViewType::e2D;
        ivci.format                          = swap_format;
        ivci.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
        ivci.subresourceRange.levelCount     = 1;
        ivci.subresourceRange.layerCount     = 1;
        swap_image_views.push_back(device.createImageView(ivci));
    }

    create_render_pass_and_pipeline(vert_spv, frag_spv);

    framebuffers.reserve(swap_image_views.size());
    for (auto view : swap_image_views) {
        vk::FramebufferCreateInfo fbci;
        fbci.renderPass      = render_pass;
        fbci.attachmentCount = 1;
        fbci.pAttachments    = &view;
        fbci.width           = swap_extent.width;
        fbci.height          = swap_extent.height;
        fbci.layers          = 1;
        framebuffers.push_back(device.createFramebuffer(fbci));
    }
}

void upload_vertex_buffer(const float* vertices, size_t byte_size) {
    // Staging (host-visible) then copy to device-local vertex buffer via graphics queue.
    VkBufferCreateInfo sb{};
    sb.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    sb.size  = byte_size;
    sb.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo sac{};
    sac.usage = VMA_MEMORY_USAGE_AUTO;
    sac.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer       staging_buf  = VK_NULL_HANDLE;
    VmaAllocation  staging_alloc = VK_NULL_HANDLE;
    if (vmaCreateBuffer(vma_allocator, &sb, &sac, &staging_buf, &staging_alloc, nullptr) !=
        VK_SUCCESS)
        fatal("vmaCreateBuffer staging failed");

    void* mapped = nullptr;
    vmaMapMemory(vma_allocator, staging_alloc, &mapped);
    std::memcpy(mapped, vertices, byte_size);
    vmaFlushAllocation(vma_allocator, staging_alloc, 0, byte_size);
    vmaUnmapMemory(vma_allocator, staging_alloc);

    VkBufferCreateInfo vb{};
    vb.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vb.size  = byte_size;
    vb.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo vac{};
    vac.usage = VMA_MEMORY_USAGE_AUTO;

    if (vmaCreateBuffer(vma_allocator, &vb, &vac, &vertex_buffer, &vertex_alloc, nullptr) !=
        VK_SUCCESS)
        fatal("vmaCreateBuffer vertex failed");

    vk::CommandBufferAllocateInfo ai;
    ai.commandPool        = command_pool;
    ai.level              = vk::CommandBufferLevel::ePrimary;
    ai.commandBufferCount = 1;
    auto cmd              = device.allocateCommandBuffers(ai).front();

    vk::CommandBufferBeginInfo bi;
    bi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmd.begin(bi);

    vk::BufferCopy region;
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size      = byte_size;
    cmd.copyBuffer(vk::Buffer(staging_buf), vk::Buffer(vertex_buffer), region);

    cmd.end();

    vk::SubmitInfo si;
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cmd;
    graphics_queue.submit(si, nullptr);
    graphics_queue.waitIdle();

    device.freeCommandBuffers(command_pool, cmd);
    vmaDestroyBuffer(vma_allocator, staging_buf, staging_alloc);
}

void record_command_buffer(std::uint32_t image_index, float time) {
    vk::CommandBuffer cmd = command_buffers[image_index];

    cmd.reset({});
    cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    vk::ClearValue clear;
    clear.color = vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 1.f});

    vk::RenderPassBeginInfo rpbi;
    rpbi.renderPass      = render_pass;
    rpbi.framebuffer     = framebuffers[image_index];
    rpbi.renderArea      = vk::Rect2D{{0, 0}, swap_extent};
    rpbi.clearValueCount = 1;
    rpbi.pClearValues    = &clear;

    cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);

    vk::Viewport viewport;
    viewport.x         = 0.f;
    viewport.y         = 0.f;
    viewport.width     = static_cast<float>(swap_extent.width);
    viewport.height    = static_cast<float>(swap_extent.height);
    viewport.minDepth  = 0.f;
    viewport.maxDepth  = 1.f;
    cmd.setViewport(0, viewport);

    vk::Rect2D scissor{{0, 0}, swap_extent};
    cmd.setScissor(0, scissor);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline);

    vk::DeviceSize offs = 0;
    cmd.bindVertexBuffers(0, {vk::Buffer(vertex_buffer)}, {offs});
    cmd.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(float),
                     &time);
    cmd.draw(3, 1, 0, 0);

    cmd.endRenderPass();
    cmd.end();
}

} // namespace

int main() {
    fprintf(stderr,
            "Wayland Vulkan demo: libdecor frame + VMA vertex buffer + swapchain triangle.\n");

    libdecor_iface.error          = libdecor_on_error;
    frame_iface.configure         = decor_frame_configure;
    frame_iface.close             = decor_frame_close;
    frame_iface.commit            = decor_frame_commit_cb;
    frame_iface.dismiss_popup     = decor_frame_dismiss_popup;

    wl_dpy = wl_display_connect(nullptr);
    if (!wl_dpy) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return EXIT_FAILURE;
    }

    wl_registry* registry = wl_display_get_registry(wl_dpy);
    wl_registry_add_listener(registry, &registry_listener, nullptr);
    wl_display_roundtrip(wl_dpy);
    wl_registry_destroy(registry);

    if (!compositor) {
        fprintf(stderr, "Missing wl_compositor\n");
        return EXIT_FAILURE;
    }

    surface = wl_compositor_create_surface(compositor);
    decor_ctx = libdecor_new(wl_dpy, &libdecor_iface);
    if (!decor_ctx)
        fatal("libdecor_new failed");

    decor_frame = libdecor_decorate(decor_ctx, surface, &frame_iface, nullptr);
    if (!decor_frame)
        fatal("libdecor_decorate failed");

    libdecor_frame_set_title(decor_frame, "Wayland Vulkan Window");
    libdecor_frame_set_app_id(decor_frame, "org.example.wayland-vulkan");
    libdecor_frame_set_capabilities(
        decor_frame,
        static_cast<libdecor_capabilities>(LIBDECOR_ACTION_MOVE | LIBDECOR_ACTION_RESIZE |
                                          LIBDECOR_ACTION_MINIMIZE | LIBDECOR_ACTION_FULLSCREEN |
                                          LIBDECOR_ACTION_CLOSE));

    libdecor_frame_map(decor_frame);
    pump_until_configured();

    // SPIR-V (compiled by CMake + glslc)
    const fs::path spv_dir(WV_SHADER_SPV_DIR);
    auto vert_spv = read_spv(spv_dir / "triangle.vert.spv");
    auto frag_spv = read_spv(spv_dir / "triangle.frag.spv");

    const std::vector<const char*> inst_ext = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
    };

    vk::ApplicationInfo app_info;
    app_info.pApplicationName   = "wayland-vulkan";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

    vk::InstanceCreateInfo ici;
    ici.pApplicationInfo      = &app_info;
    ici.enabledExtensionCount = static_cast<std::uint32_t>(inst_ext.size());
    ici.ppEnabledExtensionNames = inst_ext.data();

    instance = vk::createInstance(ici);

    vk::WaylandSurfaceCreateInfoKHR wsci;
    wsci.display = wl_dpy;
    wsci.surface = surface;

    vk_surface = instance.createWaylandSurfaceKHR(wsci);

    auto              pdevs = instance.enumeratePhysicalDevices();
    vk::PhysicalDevice chosen{};
    for (auto pd : pdevs) {
        if (find_queue_family(pd, vk_surface)) {
            chosen = pd;
            break;
        }
    }
    if (!chosen)
        fatal("no suitable Vulkan physical device / queue family");

    physical_device = chosen;
    queue_family    = *find_queue_family(physical_device, vk_surface);

    float                       queue_priority = 1.f;
    vk::DeviceQueueCreateInfo dqci;
    dqci.queueFamilyIndex = queue_family;
    dqci.queueCount       = 1;
    dqci.pQueuePriorities = &queue_priority;

    const std::vector<const char*> dev_ext = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    vk::DeviceCreateInfo dci;
    dci.queueCreateInfoCount    = 1;
    dci.pQueueCreateInfos       = &dqci;
    dci.enabledExtensionCount   = static_cast<std::uint32_t>(dev_ext.size());
    dci.ppEnabledExtensionNames = dev_ext.data();

    device         = physical_device.createDevice(dci);
    graphics_queue = device.getQueue(queue_family, 0);

    vk::CommandPoolCreateInfo cpci;
    cpci.queueFamilyIndex = queue_family;
    cpci.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    command_pool          = device.createCommandPool(cpci);

    image_available = device.createSemaphore({});
    render_finished = device.createSemaphore({});
    in_flight       = device.createFence({vk::FenceCreateFlagBits::eSignaled});

    VmaVulkanFunctions vma_vulkan_fn{};
    vma_vulkan_fn.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vma_vulkan_fn.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo aci{};
    aci.physicalDevice = static_cast<VkPhysicalDevice>(physical_device);
    aci.device         = static_cast<VkDevice>(device);
    aci.instance       = static_cast<VkInstance>(instance);
    aci.pVulkanFunctions = &vma_vulkan_fn;

    if (vmaCreateAllocator(&aci, &vma_allocator) != VK_SUCCESS)
        fatal("vmaCreateAllocator failed");

    const float vertices[] = {
        0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
    };
    upload_vertex_buffer(vertices, sizeof(vertices));

    auto anim_start = std::chrono::steady_clock::now();

    while (!closing) {
        wl_display_dispatch_pending(wl_dpy);
        libdecor_dispatch(decor_ctx, 0);

        if (swapchain_dirty) {
            create_swapchain_and_framebuffers(vert_spv, frag_spv);
            if (swapchain && swap_extent.width > 0 && swap_extent.height > 0) {
                swapchain_dirty = false;
                command_buffers = device.allocateCommandBuffers(
                    {command_pool, vk::CommandBufferLevel::ePrimary,
                     static_cast<std::uint32_t>(framebuffers.size())});
            }
            continue;
        }

        if (!swapchain || framebuffers.empty())
            continue;

        vk::Result w = device.waitForFences(in_flight, true, UINT64_MAX);
        if (w != vk::Result::eSuccess)
            fatal("waitForFences failed");

        std::uint32_t image_index = 0;
        vk::Result    acq =
            device.acquireNextImageKHR(swapchain, UINT64_MAX, image_available, {}, &image_index);
        if (acq == vk::Result::eErrorOutOfDateKHR) {
            swapchain_dirty = true;
            continue;
        }
        if (acq != vk::Result::eSuccess && acq != vk::Result::eSuboptimalKHR)
            fatal("acquireNextImageKHR failed");
        if (acq == vk::Result::eSuboptimalKHR)
            swapchain_dirty = true;

        device.resetFences(in_flight);

        float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - anim_start).count();
        record_command_buffer(image_index, t);

        vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo         submit;
        submit.waitSemaphoreCount   = 1;
        submit.pWaitSemaphores      = &image_available;
        submit.pWaitDstStageMask     = &wait_stage;
        submit.commandBufferCount   = 1;
        submit.pCommandBuffers      = &command_buffers[image_index];
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores    = &render_finished;

        graphics_queue.submit(submit, in_flight);

        vk::PresentInfoKHR present;
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores    = &render_finished;
        present.swapchainCount     = 1;
        present.pSwapchains        = &swapchain;
        present.pImageIndices      = &image_index;

        try {
            vk::Result pr = graphics_queue.presentKHR(present);
            if (pr == vk::Result::eSuboptimalKHR || pr == vk::Result::eErrorOutOfDateKHR)
                swapchain_dirty = true;
        } catch (const vk::SystemError& e) {
            if (e.code() == vk::Result::eErrorOutOfDateKHR)
                swapchain_dirty = true;
        }
    }

    device.waitIdle();

    vmaDestroyBuffer(vma_allocator, vertex_buffer, vertex_alloc);
    vmaDestroyAllocator(vma_allocator);

    device.destroyFence(in_flight);
    device.destroySemaphore(render_finished);
    device.destroySemaphore(image_available);
    destroy_swapchain_and_graphics();
    device.destroyCommandPool(command_pool);
    device.destroy();

    instance.destroySurfaceKHR(vk_surface);
    instance.destroy();

    libdecor_frame_unref(decor_frame);
    wl_surface_destroy(surface);
    libdecor_unref(decor_ctx);
    wl_compositor_destroy(compositor);
    wl_display_disconnect(wl_dpy);
    return EXIT_SUCCESS;
}
