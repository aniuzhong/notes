# Visual Studio

- [Visual Studio](#visual-studio)
  - [Download](#download)
  - [Product Key](#product-key)
  - [DLL Project](#dll-project)
  - [Qt Project](#qt-project)
    - [安装 Qt VS Tools 扩展](#安装-qt-vs-tools-扩展)
    - [配置 Qt Designer 在独立窗口中运行](#配置-qt-designer-在独立窗口中运行)
    - [在 Visual Studio（Qt VS Tools）里勾选需要的 Modules](#在-visual-studioqt-vs-tools里勾选需要的-modules)
  - [Search (Ctrl + T)](#search-ctrl--t)
  - [Nuget](#nuget)
  - [Extensions](#extensions)
    - [VsVim 2022](#vsvim-2022)
  - [添加额外头文件](#添加额外头文件)
  - [添加额外库](#添加额外库)
  - [引入额外编译参数](#引入额外编译参数)
    - [启用 `/Zc:__cplusplus`](#启用-zc__cplusplus)
    - [编译 `spdlog` 或 `{fmt}` 添加 UTF-8 支持](#编译-spdlog-或-fmt-添加-utf-8-支持)
  - [注意事项](#注意事项)

## Download

[Older versions entry point](https://visualstudio.microsoft.com/vs/older-downloads/)

> Visual Studio 2017 和 2019 下载链接对于非付费账户的官网下载入口已经不可用，但还可以直接复制粘贴下面的下载链接文本到浏览器下载：

- [Visual Studio 2017 Community](https://aka.ms/vs/15/release/vs_community.exe)
- [Visual Studio 2017 Professional](https://aka.ms/vs/15/release/vs_professional.exe)
- [Visual Studio 2017 Enterprise](https://aka.ms/vs/15/release/vs_enterprise.exe)
- [Visual Studio 2019 Community](https://aka.ms/vs/16/release/vs_community.exe)
- [Visual Studio 2019 Professional](https://aka.ms/vs/16/release/vs_professional.exe)
- [Visual Studio 2019 Enterprise](https://aka.ms/vs/16/release/vs_enterprise.exe)
- [Visual Studio 2022 Community](https://aka.ms/vs/17/release/vs_community.exe)
- [Visual Studio 2022 Professional](https://aka.ms/vs/17/release/vs_professional.exe)
- [Visual Studio 2022 Enterprise](https://aka.ms/vs/17/release/vs_enterprise.exe)

## Product Key

- Enterprise 2026: VYGRN-WPR22-HG4X3-692BF-QGT2V
- Enterprise 2022: VHF9H-NXBBB-638P6-6JHCY-88JWH

`Help`->`About Microsoft Visual Studio`->`License status`->`Unlock with a Product Key`

## DLL Project

在 Visual Studio 新建 DLL 项目时，默认生成的 `pch.h / pch.cpp` 和 `framework.h` 文件主要是为了使用 `预编译头 (Precompiled Header, PCH)` 功能。

**小型 DLL 工程**可以删除这些文件并关闭预编译头，保持代码简洁。

- 右键项目 -> `Properties`
- 打开 `Configuration Properties` -> `C/C++` -> `Precompiled Headers`
- 将 `Precompiled Header` 设置为 **`Not Using Precompiled Headers`**
- 删除默认生成的 `pch.h / pch.cpp` 和 `framework.h`

DLL 工程默认会有 `<项目名>_EXPORTS` 宏，用来控制 `__declspec(dllexport)` 和 `__declspec(dllimport)`。

Visual Studio 在编译 DLL 时，除了生成 .dll 文件，还会生成一个 导入库 (.lib)。

这个 .lib 文件的作用是：

- 如果调用方选择 静态链接方式（在编译时就知道要用 DLL），编译器会用 .lib 来解析符号，运行时再加载 .dll。
- 它本质上是一个“符号占位库”，里面没有真正的实现，只是告诉链接器 DLL 里有哪些函数。

如果团队约定通过 **ABI + 动态加载**，即运行时用 `LoadLibrary` + `GetProcAddress`，那么 .lib 文件 不需要使用。可以忽略它，只分发 .dll 和对应的头文件（或者 ABI 文档）

## Qt Project

### 安装 Qt VS Tools 扩展

这是在 Visual Studio 中进行 Qt 开发的基石。

1. 打开 Visual Studio。
2. 前往菜单栏 扩展 (Extensions) > 管理扩展 (Manage Extensions)。
3. 在打开的对话框中，搜索框输入 “Qt Visual Studio Tools”。
4. 找到该扩展并点击 “安装 (Install)”。安装完成后可能需要重启 Visual Studio。
5. 确保该扩展已启用。

### 配置 Qt Designer 在独立窗口中运行

确保 Visual Studio 以最稳定、兼容的方式调用外部的 Qt Designer 程序，避免其在内部集成 Qt Designer 时打开 UI 文件闪退。

1. 在 Visual Studio 中，前往菜单栏 **扩展 (Extensions)** > **Qt VS Tools** > **Qt 选项 (Qt Options)**。
2. 在弹出的 **“Qt Options”** 对话框中，左侧导航栏选择 **“Qt”** 下的 **“General”**。
3. 在右侧的设置区域，找到 **“Qt Designer”** 选项组。
4. 将 **“Run in detached window”** 选项的值设置为 `True`。
    - 解释： 这个设置告诉 Visual Studio 在打开 .ui 文件时，不要尝试将 Qt Designer 的界面嵌入到 Visual Studio 内部，而是启动一个完全独立的 Qt Designer 进程和窗口。这种方式通常能有效避免因集成模式引发的兼容性或稳定性问题。
5. 点击 **“应用 (Apply)”**，然后点击 **“确定 (OK)”** 保存设置。
6. **强烈建议重启 Visual Studio**，以确保所有更改生效。

### 在 Visual Studio（Qt VS Tools）里勾选需要的 Modules

1. 打开 **Qt Project Settings** → **Qt Modules**
2. 勾选 Core，点击 OK
3. 在 Visual Studio 里执行 Clean / Rebuild，或在使用 qmake 的项目上运行一次 qmake（或对 CMake 项目重新配置/生成）
4. 在代码中使用相应头文件测试编译。

## Search (<kbd>Ctrl</kbd> + <kbd>T</kbd>)

## Nuget

`Tools` -> `NuGet Package Manager` -> `Manage NuGet Packages for Soltution...`

## Extensions

### VsVim 2022

`%USERPROFILE%\_vimrc`

## 添加额外头文件

- `Configuration Properties` > `C/C++` > `All Options` > `Additional Include Directories`

## 添加额外库

- `Configuration Properties` > `Linker` > `All Options` > `Additional Library Directories`
- `Configuration Properties` > `Linker` > `All Options` > `Additional Dependencies`

## 引入额外编译参数

- 右键项目 -> `Properties`
- 打开 `Configuration Properties` -> `C/C++` -> `Command Line`
- `Additional Options` 处输入要添加的参数

### 启用 `/Zc:__cplusplus`

- MSVC 不会按照标准推荐值设置`__cplusplus`，除非启用 `/Zc:__cplusplus`

### 编译 `spdlog` 或 `{fmt}` 添加 UTF-8 支持

- `/utf-8`

## 注意事项

- Visual Studio 现在的 IntelliSense 非常智能，当输入 #include < 后，它弹出的自动补全列表通常就是磁盘上**真实的文件名大小写**。直接回车确认。
