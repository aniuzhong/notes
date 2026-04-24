# FFmpeg

- [FFmpeg](#ffmpeg)
  - [Compiling FFmpeg on Windows using MSVC](#compiling-ffmpeg-on-windows-using-msvc)
    - [**★vcpkg**](#vcpkg)
    - [**ShiftMediaProject(deprecated)**](#shiftmediaprojectdeprecated)
  - [C++ helpers](#c-helpers)
  - [FFplay](#ffplay)
    - [无法播放 HEIC 图片](#无法播放-heic-图片)

## Compiling FFmpeg on Windows using MSVC

- 在 Windows 下把 ffmpeg 当作**源码级第三方依赖**几乎不可行，最佳实践还是**预编译二进制管理**。

- 修改 FFmpeg APIs 源码后，应先用 FFplay 测试，之后再和实际项目联调。

### [**★vcpkg**](https://github.com/microsoft/vcpkg)

vcpkg 会使用 **msvc 工具链**对 FFmpeg 及其依赖（指定开启的特性）进行编译。编译过程中**所使用源码均来自官方**。

vcpkg 的 ffmpeg port 位于 `D:\vcpkg\ports\ffmpeg\vcpkg.json`

一些库没有直接打包成 vcpkg 的 feature。 因为它们的应用场景已经不再是主流，或者有更现代的替代方案：

- libcdio：主要用于 CD/DVD 光盘抓取和播放。随着光驱逐渐消失，使用场景越来越少。
- libbluray：Blu-ray 光盘支持，仍然有用，但只对需要直接处理蓝光盘的用户重要。
- libxvid：早期的 MPEG-4 Part 2 编码器，如今几乎完全被 H.264（x264）和 H.265（x265）取代。
- libgme：用于播放老游戏机音乐文件（NES、SNES、Genesis 等），属于爱好者或特定项目需求。
- libgcrypt：通用加密库，FFmpeg 可以用它来实现 TLS/SSL，但更常见的选择是 openssl 或 gnutls。
- schannel：Windows 自带的 TLS 后端，自动启用，不需要额外安装，但它只在 Windows 上有意义。

在 Windows 下构建命令如下（尽量启用最多的特性）：

```Powershell
# alsa、vaapi 在构建时也可以强制指定，但在 Windows 平台下没有实际意义。
vcpkg.exe install ffmpeg[all,all-gpl,all-nonfree,dvdvideo,drawtext,zmq,rubberband,ffplay,ffmpeg,ffprobe]
```

编译完成后（Release 版本）运行 `ffplay.exe -version` 应该得出类似下面的输出：

```
ffplay version 8.1 Copyright (c) 2003-2026 the FFmpeg developers
built with Microsoft (R) C/C++ Optimizing Compiler Version 19.44.35222 for x64
configuration: --prefix='D:/vcpkg/packages/ffmpeg_x64-windows' --toolchain=msvc --enable-pic --disable-doc --enable-runtime-cpudetect --disable-autodetect --target-os=win32 --enable-w32threads --enable-d3d11va --enable-d3d12va --enable-dxva2 --enable-mediafoundation --cc=cl.exe --cxx=cl.exe --windres=rc.exe --ld=link.exe --ar='ar-lib lib.exe' --ranlib=':' --enable-nonfree --enable-gpl --enable-version3 --enable-ffmpeg --enable-ffplay --enable-ffprobe --enable-avcodec --enable-avdevice --enable-avformat --enable-avfilter --enable-swresample --enable-swscale --disable-alsa --enable-amf --enable-libaom --enable-libass --enable-avisynth --enable-bzlib --enable-libdav1d --enable-libfdk-aac --enable-libfontconfig --enable-libharfbuzz --enable-libfreetype --enable-libfribidi --enable-iconv --enable-libilbc --enable-lzma --enable-libmp3lame --enable-libmodplug --enable-cuda --enable-nvenc --enable-nvdec --enable-cuvid --enable-ffnvcodec --enable-opencl --enable-opengl --enable-libopenh264 --enable-libopenjpeg --enable-libopenmpt --enable-openssl --enable-libopus --enable-sdl2 --enable-libsnappy --enable-libsoxr --enable-libspeex --enable-libssh --disable-libtensorflow --enable-libtesseract --enable-libtheora --enable-libvorbis --enable-libvpx --enable-vulkan --enable-libwebp --enable-libx264 --enable-libx265 --enable-libxml2 --enable-zlib --enable-libsrt --enable-libmfx --enable-encoder=h264_qsv --enable-decoder=h264_qsv --disable-vaapi --enable-libzmq --enable-librubberband --enable-cross-compile --disable-static --enable-shared --extra-cflags='-DHAVE_UNISTD_H=0' --pkg-config='D:/vcpkg/downloads/tools/msys2/3e71d1f8e22ab23f/mingw64/bin/pkg-config.exe' --enable-optimizations --extra-cflags=-MD --extra-cxxflags=-MD --extra-ldflags='-libpath:D:/vcpkg/installed/x64-windows/lib' --arch=x86_64 --enable-asm --enable-x86asm
libavutil      60. 26.100 / 60. 26.100
libavcodec     62. 28.100 / 62. 28.100
libavformat    62. 12.100 / 62. 12.100
libavdevice    62.  3.100 / 62.  3.100
libavfilter    11. 14.100 / 11. 14.100
libswscale      9.  5.100 /  9.  5.100
libswresample   6.  3.100 /  6.  3.100
```

| 编译参数 | 含义 |
|---|---|
| `--toolchain=msvc` | 使用 Microsoft Visual C++ 工具链编译 |
| `--enable-pic` | 启用位置无关代码（PIC），便于共享库 |
| `--disable-doc` | 不生成文档 |
| `--enable-runtime-cpudetect` | 运行时检测 CPU 特性 |
| `--disable-autodetect` | 禁用自动探测外部库 |
| `--target-os=win32` | 目标操作系统为 Windows |
| `--cc=cl.exe` | 使用 cl.exe 作为 C 编译器 |
| `--cxx=cl.exe` | 使用 cl.exe 作为 C++ 编译器 |
| `--windres=rc.exe` | 使用 rc.exe 处理资源文件 |
| `--ld=link.exe` | 使用 link.exe 作为链接器 |
| `--enable-w32threads` | 启用 Windows 线程支持 |
| `--enable-cross-compile` | 启用交叉编译 |
| `--disable-static` | 禁用静态库，生成动态库 |
| `--enable-shared` | 启用共享库 |
| `--extra-cflags='-DHAVE_UNISTD_H=0'` | 指定额外编译参数，禁用 unistd.h |
| `--enable-optimizations` | 启用优化 |
| `--extra-cflags=-MD` | 使用 MSVC 多线程 DLL 运行时 |
| `--extra-cxxflags=-MD` | 使用 MSVC 多线程 DLL 运行时（C++） |
| `--arch=x86_64` | 架构为 64 位 |
| `--enable-asm` | 启用汇编优化 |
| `--enable-x86asm` | 启用 x86 汇编优化 |
| `--ar='ar-lib lib.exe'` | 使用 lib.exe 作为归档工具 |
| `--enable-d3d11va` | 启用 Direct3D 11 视频加速 |
| `--enable-d3d12va` | 启用 Direct3D 12 视频加速 |
| `--enable-dxva2` | 启用 DirectX 视频加速 2 |
| `--enable-mediafoundation` | 启用 Windows Media Foundation 支持 |
| `--enable-nonfree` | 启用非自由组件（如 fdk-aac） |
| `--enable-gpl` | 启用 GPL 许可代码 |
| `--enable-version3` | 启用 LGPL/GPL 第 3 版代码 |
| `--enable-ffmpeg` | 启用 ffmpeg 命令行工具 |
| `--enable-ffplay` | 启用 ffplay 命令行工具 |
| `--enable-ffprobe` | 启用 ffprobe 命令行工具 |
| `--enable-avcodec` | 启用编解码库 |
| `--enable-avdevice` | 启用设备输入输出支持 |
| `--enable-avformat` | 启用封装格式支持 |
| `--enable-avfilter` | 启用滤镜功能 |
| `--enable-swresample` | 启用音频重采样 |
| `--enable-swscale` | 启用视频缩放 |
| `--enable-amf` | 启用 AMD AMF 编码接口 |
| `--enable-libaom` | 启用 AV1 编码库 libaom |
| `--enable-libass` | 启用字幕渲染库 libass |
| `--enable-avisynth` | 启用 AviSynth 脚本支持 |
| `--enable-bzlib` | 启用 bzip2 压缩库 |
| `--enable-libdav1d` | 启用 AV1 解码库 dav1d |
| `--enable-libfdk-aac` | 启用 Fraunhofer FDK AAC 编码库 |
| `--enable-libfontconfig` | 启用字体配置库 |
| `--enable-libfreetype` | 启用 FreeType 字体渲染 |
| `--enable-libfribidi` | 启用双向文本支持 |
| `--enable-libharfbuzz` | 启用 Harfbuzz 字体库|
| `--enable-iconv` | 启用字符编码转换 |
| `--enable-libilbc` | 启用 iLBC 编码库 |
| `--enable-lzma` | 启用 LZMA 压缩库 |
| `--enable-libmp3lame` | 启用 MP3 编码库 LAME |
| `--enable-libmodplug` | 启用 MOD 音乐文件支持 |
| `--enable-cuda` | 启用 CUDA 支持 |
| `--enable-nvenc` | 启用 NVIDIA NVENC 编码 |
| `--enable-nvdec` | 启用 NVIDIA NVDEC 解码 |
| `--enable-cuvid` | 启用 NVIDIA CUVID 解码 |
| `--enable-ffnvcodec` | 启用 NVIDIA FFmpeg 编解码接口 |
| `--enable-opencl` | 启用 OpenCL 支持 |
| `--enable-opengl` | 启用 OpenGL 支持 |
| `--enable-libopenh264` | 启用 Cisco OpenH264 编码库 |
| `--enable-libopenjpeg` | 启用 JPEG2000 编码库 |
| `--enable-libopenmpt` | 启用 OpenMPT 音乐模块支持 |
| `--enable-openssl` | 启用 OpenSSL 支持 |
| `--enable-libopus` | 启用 Opus 音频编解码 |
| `--enable-sdl2` | 启用 SDL2 图形/音频支持 |
| `--enable-libsnappy` | 启用 Snappy 压缩库 |
| `--enable-libsoxr` | 启用高质量音频重采样库 |
| `--enable-libspeex` | 启用 Speex 音频编解码 |
| `--enable-libssh` | 启用 SSH 支持 |
| `--enable-libtesseract` | 启用 OCR 库 Tesseract |
| `--enable-libtheora` | 启用 Theora 视频编解码 |
| `--enable-libvorbis` | 启用 Vorbis 音频编解码 |
| `--enable-libvpx` | 启用 VP8/VP9 编解码 |
| `--enable-vulkan` | 启用 Vulkan 图形接口 |
| `--enable-libwebp` | 启用 WebP 图像编解码 |
| `--enable-libx264` | 启用 H.264 编码库 x264 |
| `--enable-libx265` | 启用 H.265/HEVC 编码库 x265 |
| `--enable-libxml2` | 启用 XML 解析库 |
| `--enable-zlib` | 启用 zlib 压缩库 |
| `--enable-libsrt` | 启用 SRT 协议支持 |
| `--enable-libmfx` | 启用 Intel Media SDK (QSV) |
| `--enable-encoder=h264_qsv` | 启用 Intel QSV H.264 编码器 |
| `--enable-decoder=h264_qsv` | 启用 Intel QSV H.264 解码器 |
| `--enable-libzmq` | 启用 ZeroMQ 消息库 |
| `--enable-librubberband` | 启用 Rubberband 音频变速库 |
| `--ranlib=':'` | ranlib 无操作（Windows 不需要） |
| `--disable-vaapi` | 禁用 VAAPI（Linux 视频加速接口） |
| `--disable-alsa` | 禁用 ALSA（Linux 音频系统） |

> 如果手工复现 vcpkg 的编译，工作量会十分可观。

### [**ShiftMediaProject(deprecated)**](https://shiftmediaproject.github.io/)

非官方。作者已经停止维护这个项目，并建议使用 vcpkg。最后一个版本是 FFmpeg 7.1。

但如果需要在 Visual Studio 中 **修改 API 源码**，**调试 ffplay、ffmpeg 等工具**，这个工程仍然是最方便的。

## C++ helpers

https://www.mail-archive.com/ffmpeg-user@ffmpeg.org/msg30274.html

``` C++
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
}

#ifdef av_err2str
#undef av_err2str
#include <string>
av_always_inline std::string av_err2string(int errnum)
{
    char str[AV_ERROR_MAX_STRING_SIZE];
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#define av_err2str(err) av_err2string(err).c_str()
#endif // av_err2str

#ifdef av_ts2timestr
#undef av_ts2timestr
#include <string>
av_always_inline std::string av_ts2timestring(int64_t ts, const AVRational* tb)
{
    char str[AV_ERROR_MAX_STRING_SIZE];
    return av_ts_make_time_string2(str, ts, *tb);
}
#define av_ts2timestr(ts, tb) av_ts2timestring(ts, tb).c_str()
#endif // av_ts2timestr
```

## FFplay

> FFplay is a very simple and portable media player using the FFmpeg libraries and the SDL library. It is mostly used as a testbed for the various FFmpeg APIs.

- FFplay 定位是 FFmpeg APIs 的测试工具。

### 无法播放 HEIC 图片

``` shell
ffplay "/Users/aniu/Library/Application Support/com.apple.mobileAssetDesktop/Catalina Rock.heic"
```
