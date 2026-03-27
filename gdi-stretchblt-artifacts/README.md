# gdi-stretchblt-artifacts

## 问题描述

当图像被 `StretchBlt` 缩小或放大时，如不显式调用`SetStretchBltMode`），高频细节容易产生混叠（aliasing）。

## 使用方法

1. 使用 CMake preset 配置并编译：
   ```powershell
   cmake --preset vs2022-x64
   cmake --build --preset build-release
   ```
2. 运行：
   ```powershell
   .\build\vs2022-x64\Release\gdi-stretchblt-artifacts.exe
   ```
3. 初始窗口中间会显示 `Open bitmap` 按钮。
4. 点击按钮，选择一个 `.bmp` 文件。
5. 加载成功后：
   - 窗口客户区会自动调整为图片原始尺寸。
   - 后续拖拽窗口大小时，图片会始终铺满客户区。
6. 在窗口内右键，打开拉伸模式菜单并切换：
   - `NONE`：不调用 `SetStretchBltMode`
   - `COLORONCOLOR`
   - `HALFTONE`
