cmake_minimum_required(VERSION 3.24)
project(abc-demos LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

# ═══════════════════════════════════════════════════════════════════════════════
#  Common: X11
# ═══════════════════════════════════════════════════════════════════════════════
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)

# ═══════════════════════════════════════════════════════════════════════════════
#  Target: x11-glx-gl  (X11 + GLX + OpenGL 3.3 Core)
# ═══════════════════════════════════════════════════════════════════════════════
pkg_check_modules(GL REQUIRED IMPORTED_TARGET gl)

add_executable(x11-glx-gl x11_glx_gl.cpp)
target_link_libraries(x11-glx-gl PRIVATE PkgConfig::X11 PkgConfig::GL)

# ═══════════════════════════════════════════════════════════════════════════════
#  Target: x11-egl-gl  (X11 + EGL + OpenGL 3.3 Core)
# ═══════════════════════════════════════════════════════════════════════════════
pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)

add_executable(x11-egl-gl x11_egl_gl.cpp)
target_link_libraries(x11-egl-gl PRIVATE PkgConfig::X11 PkgConfig::EGL PkgConfig::GL)

# ═══════════════════════════════════════════════════════════════════════════════
#  Target: x11-egl-gles  (X11 + EGL + OpenGL ES 2.0)
# ═══════════════════════════════════════════════════════════════════════════════
pkg_check_modules(GLESv2 REQUIRED IMPORTED_TARGET glesv2)

add_executable(x11-egl-gles x11_egl_gles.cpp)
target_link_libraries(x11-egl-gles PRIVATE PkgConfig::X11 PkgConfig::EGL PkgConfig::GLESv2)

# ═══════════════════════════════════════════════════════════════════════════════
#  Common: Wayland — window frame (title bar, min/max/close) via libdecor (not xdg-decoration SSD).
#
# If wayland-info lists no zxdg_decoration_manager_v1, requesting server-side decorations alone is
# ineffective; link libdecor instead.
# Debian/Ubuntu: sudo apt install libdecor-0-dev libdecor-0-plugin-1-cairo
# (Plugins draw the decorations; -dev alone may not show a frame at runtime.)
# ═══════════════════════════════════════════════════════════════════════════════
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(WAYLAND_EGL  REQUIRED IMPORTED_TARGET wayland-egl)
pkg_check_modules(LIBDECOR   REQUIRED IMPORTED_TARGET libdecor-0)

# ═══════════════════════════════════════════════════════════════════════════════
#  Target: wayland-egl-gl  (Wayland + EGL + OpenGL 3.3 Core)
# ═══════════════════════════════════════════════════════════════════════════════
add_executable(wayland-egl-gl wayland_egl_gl.cpp)
target_link_libraries(wayland-egl-gl PRIVATE
    PkgConfig::LIBDECOR
    PkgConfig::WAYLAND_CLIENT
    PkgConfig::WAYLAND_EGL
    PkgConfig::EGL
    PkgConfig::GL
)

# ═══════════════════════════════════════════════════════════════════════════════
#  Target: wayland-egl-gles  (Wayland + EGL + OpenGL ES 2.0)
# ═══════════════════════════════════════════════════════════════════════════════
add_executable(wayland-egl-gles wayland_egl_gles.cpp)
target_link_libraries(wayland-egl-gles PRIVATE
    PkgConfig::LIBDECOR
    PkgConfig::WAYLAND_CLIENT
    PkgConfig::WAYLAND_EGL
    PkgConfig::EGL
    PkgConfig::GLESv2
)

# ═══════════════════════════════════════════════════════════════════════════════
#  Target: wayland-vulkan  (Wayland + libdecor + Vulkan-Hpp swapchain + VMA vertex buffer)
# ═══════════════════════════════════════════════════════════════════════════════
find_package(Vulkan REQUIRED)

find_program(GLSLC glslc
  HINTS "${Vulkan_GLSLC_EXECUTABLE}" "$ENV{VULKAN_SDK}/bin"
  DOC "SPIR-V compiler from the Vulkan SDK or shaderc"
)
if(NOT GLSLC)
  message(FATAL_ERROR "glslc not found. Install the `glslc` package or the Vulkan SDK (shaderc tools).")
endif()

include(FetchContent)
FetchContent_Declare(
  VulkanMemoryAllocator
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG        v3.1.0
)
FetchContent_MakeAvailable(VulkanMemoryAllocator)

set(WV_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wayland_vulkan_shaders)
set(WV_SPV_DIR ${CMAKE_CURRENT_BINARY_DIR}/wayland_vulkan_spv)
file(MAKE_DIRECTORY ${WV_SPV_DIR})

set(WV_VERT_SPV ${WV_SPV_DIR}/triangle.vert.spv)
set(WV_FRAG_SPV ${WV_SPV_DIR}/triangle.frag.spv)

add_custom_command(
  OUTPUT ${WV_VERT_SPV} ${WV_FRAG_SPV}
  COMMAND ${GLSLC} -fshader-stage=vert ${WV_SHADER_DIR}/triangle.vert -o ${WV_VERT_SPV}
  COMMAND ${GLSLC} -fshader-stage=frag ${WV_SHADER_DIR}/triangle.frag -o ${WV_FRAG_SPV}
  DEPENDS ${WV_SHADER_DIR}/triangle.vert ${WV_SHADER_DIR}/triangle.frag
  COMMENT "Compile wayland_vulkan shaders (SPIR-V)"
)

add_executable(wayland-vulkan wayland_vulkan.cpp ${WV_VERT_SPV} ${WV_FRAG_SPV})
target_compile_definitions(wayland-vulkan PRIVATE WV_SHADER_SPV_DIR="${WV_SPV_DIR}")
target_link_libraries(wayland-vulkan PRIVATE
  Vulkan::Vulkan
  VulkanMemoryAllocator
  PkgConfig::LIBDECOR
  PkgConfig::WAYLAND_CLIENT
)

# ═══════════════════════════════════════════════════════════════════════════════
#  Summary message
# ═══════════════════════════════════════════════════════════════════════════════
message(STATUS "Targets: x11-glx-gl  x11-egl-gl  x11-egl-gles  wayland-egl-gl  wayland-egl-gles  wayland-vulkan")
