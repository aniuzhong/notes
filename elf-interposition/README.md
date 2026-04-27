# 动态加载符号冲突问题

- [动态加载符号冲突问题](#动态加载符号冲突问题)
  - [问题描述](#问题描述)

## 问题描述

工程代码节选自实际项目。

根据目前试验结果，和该问题是否发生无关的是：

- `-fPIC` 参数
- `dlopen` 函数的参数
- 生成 `libXxxxPlayer.so` 时是否加 `-Bsymbolic` 参数

复现问题的条件：

- 针对 `libXxxxPlayer.so` 采用**动态链接（load-time dynamic linking）**
- 针对 `libXxxxAudioCaptureCore.so` 采用**显式动态加载（runtime dynamic loading）**
- 生成 `libXxxxAudioCaptureCore.so` **不**加 `-Bsymbolic` 参数

在项目中，注释掉 CMakeLists.txt 中的

```cmake
target_link_options(XxxxAudioCaptureCore PRIVATE "-Wl,-Bsymbolic")
```

运行 XxxxPlayerDemo 会看到

```
NACSetLogCallbackImpl: 0x7fd648d252d8
Segmentation fault (core dumped)
```