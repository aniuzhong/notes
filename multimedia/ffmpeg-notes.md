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
