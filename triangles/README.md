# Hello Triangles

每个 demo 由三元组构成：**窗口平台 + 上下文桥接层 + 图形 API**。

| 窗口平台 | 上下文 / 桥接层 | 图形 API | 文件 | 备注 |
|---|---|---|---|---|
| **Linux** | | | | |
| X11 | GLX | OpenGL | `linux/x11_glx_gl.cpp` | 经典原生 |
| X11 | EGL | OpenGL | `linux/x11_egl_gl.cpp` | EGL 替代 GLX |
| X11 | EGL | OpenGL ES | `linux/x11_egl_gles.cpp` | 嵌入式 GL |
| X11 | — | Vulkan | *(未实现)* | `VK_KHR_xlib_surface` |
| Wayland | EGL | OpenGL | `linux/wayland_egl_gl.cpp` | 现代显示服务端 |
| Wayland | EGL | OpenGL ES | `linux/wayland_egl_gles.cpp` | 现代+嵌入式 |
| Wayland | — | Vulkan | `linux/wayland_vulkan.cpp` | 趋势 |
| **Windows** | | | | |
| Win32 | WGL | OpenGL | *(未实现)* | 老旧，仅 NVIDIA 驱动靠谱 |
| Win32 | EGL | OpenGL | *(未实现)* | EGL on Windows |
| Win32 | EGL | OpenGL ES | *(未实现)* | ANGLE 路径（Chrome/VS Code） |
| Win32 | — | Vulkan | *(未实现)* | `VK_KHR_win32_surface`，跨平台 |
| Win32 | — | D3D11 | *(未实现)* | Windows 原生一等公民 |
| Win32 | — | D3D12 | `windows/win32_d3d12.cpp` | Windows 原生一等公民 |
| **macOS** | | | | |
| Cocoa (NSWindow) | CGL | OpenGL | *(未实现)* | 已废弃（10.14 起），仅兼容旧版 |
| Cocoa (NSWindow) | — | Metal | *(未实现)* | Apple 原生一等公民 |
| Cocoa (NSWindow) | MoltenVK | Vulkan | *(未实现)* | 翻译层：Vulkan -> Metal |
| **iOS** | | | | |
| UIKit (UIView) | EAGL | OpenGL ES | *(未实现)* | 已废弃（iOS 12 起） |
| UIKit (UIView) | — | Metal | *(未实现)* | Apple 移动端原生一等公民 |
| UIKit (UIView) | MoltenVK | Vulkan | *(未实现)* | 翻译层：Vulkan -> Metal |
| **Android** | | | | |
| ANativeWindow | EGL | OpenGL ES | *(未实现)* | Android 标准 GL 方案 |
| ANativeWindow | — | Vulkan | *(未实现)* | `VK_KHR_android_surface`，现代首选 |

## 上下文/桥接层

| 类型 | 成员 | 说明 |
|---|---|---|
| 原生 GL 上下文 API | GLX、WGL、CGL、EAGL | 各平台自带的 GL 上下文创建 API，与窗口系统强绑定，不可跨平台 |
| 跨平台 GL 上下文 API | EGL (Khronos) | 同时支持桌面 GL 和 GLES，Linux / Windows / Android 通用，Apple 平台不支持 |
| 无桥接层 | Vulkan、D3D11、D3D12、Metal | 自带设备创建，仅通过一个 surface 扩展与窗口系统对接 |
| 翻译层 | MoltenVK、ANGLE | 将一种图形 API 调用翻译为另一种，运行在宿主 API 之上 |

> Vulkan 自己就包含了上下文创建，不需要额外的中间层。

唯一跟平台有关系的只有 surface 扩展：

- Linux: VK_KHR_wayland_surface / VK_KHR_xlib_surface
- Windows: VK_KHR_win32_surface

> D3D11/D3D12 也没有桥阶层。通过 DXGI 直接拿 swapchain，是 Windows 的一等公民。