# Gaming

- [Gaming](#gaming)
  - [EA 端口 3216 被占用问题解决方案](#ea-端口-3216-被占用问题解决方案)
    - [第一步：排查端口占用情况](#第一步排查端口占用情况)
    - [第二步：检查动态端口范围](#第二步检查动态端口范围)
    - [第三步：修改动态端口范围](#第三步修改动态端口范围)
  - [Steam 平台 `暴雨` Xbox 手柄右摇杆旋转不灵敏的解决方案](#steam-平台-暴雨-xbox-手柄右摇杆旋转不灵敏的解决方案)
  - [Steam 平台 `极乐迪斯科` Xbox 手柄没有切换语言键位的解决方法](#steam-平台-极乐迪斯科-xbox-手柄没有切换语言键位的解决方法)
  - [Windows 平台 `看火人` (Firewatch) 晕 3D 解决方案 (来自 Steam 社区玩家)](#windows-平台-看火人-firewatch-晕-3d-解决方案-来自-steam-社区玩家)
    - [修改 Firewatch 中的 FOV](#修改-firewatch-中的-fov)
  - [Wallpaper Engine 动态壁纸导出图片](#wallpaper-engine-动态壁纸导出图片)

## EA 端口 3216 被占用问题解决方案

```
通信错误

应用程序无法连接到端口 3216，这可能会阻止游戏启动。您可以
继续使用此应用程序的有限功能，或者查看我们的帮助文章，获取
疑难解答提示。

错误代码： EC:10701
```

端口 3216 被占用通常与 Windows 的虚拟化功能（如 Hyper-V）有关。Hyper-V 会保留一部分端口供宿主机和虚拟机通信使用，这可能导致特定端口无法被其他程序绑定。

可以通过修改 Windows 动态端口范围来避开 3216 端口，在保留虚拟机功能情况下解决此问题。

### 第一步：排查端口占用情况

使用**管理员权限**打开终端，输入以下命令并回车：

```powershell
netsh interface ipv4 show excludedportrange protocol=tcp
```

查看输出列表，检查端口 3216 是否处于某一行“开始端口”和“结束端口”的范围区间内。

- 是：说明该端口确实被系统保留（excluded）了。
- 否：如果不在范围内却提示占用，可能涉及其他极其罕见的网络服务问题。

### 第二步：检查动态端口范围

继续在终端中输入：

```powershell
netsh int ip show dynamicport tcp
```

如果显示的“启动端口”数值**小于 3216**，说明系统将 3216 包含在了动态分配的端口池中，导致被随机占用。

### 第三步：修改动态端口范围

为了释放 3216，我们需要将动态端口的起始位置向后移动（建议设置为国际标准的 49152）。

在终端中输入以下命令：

```powershell
netsh int ip set dynamicport tcp start=49152 num=16384
```

执行成功后，重启电脑以刷新网络设置。重启后，3216 端口应该就被释放出来了。

## Steam 平台 `暴雨` Xbox 手柄右摇杆旋转不灵敏的解决方案

1. 在 Steam 库中点击 `Heavy Rain`；
2. 连接 🎮，点击右边 `View controller settings`；
3. 通常当前布局为 `Heavy Rain` 的官方布局（`Official Layout for Heavy Rain - Official Configuration`），点击 `Edit Layout`；
4. 选择 `Joysticks`，在 `Deadzones` 中将 `Deadzone Source` 设置为 `Custom`;
5. 最后将 `Deadzone Shape` 改为 `Square`。

## Steam 平台 `极乐迪斯科` Xbox 手柄没有切换语言键位的解决方法

在 Steam 映射里添加一个指令，建议用按键的菜单按钮里的选择。

1. 在 Steam 库中点击 `Disco Elysium`；
2. 连接 🎮，点击右边 `View controller settings`；
3. 点击 `Edit Layout`；
4. 点击 MENU BUTTON 中的 `Select` 键，`Settings` > `Add extra command` > `Keyboard` > `Q`

## Windows 平台 `看火人` (Firewatch) 晕 3D 解决方案 (来自 Steam 社区玩家)

Firewatch 的默认 FOV (field of view) 是 55，这个数值比很多第一人称游戏都小很多，所以如果玩 Firewatch 时感觉头晕可以选择调整一下 FOV。

### 修改 Firewatch 中的 FOV

1. 按 `Win + R` 打开“运行”，输入 `regedit.exe` 并回车，打开注册表编辑器。
2. 导航至以下路径：`HKEY_CURRENT_USER\Software\CampoSanto\Firewatch`
3. 找到键值 `fovAdjust_h2041137991`，右键选择“修改”。
4. 将“基数”设为“十进制”。

**FOV 计算方式**

游戏默认 FOV 为 55。输入的数值 =（目标 FOV − 55）× 100。示例：

- 目标 90 → 输入 `3500`（(90−55)×100）
- 目标 40 → 输入 `-1500`（(40−55)×100）

**注意事项**

- 修改前请确保 Firewatch 未运行。
- FOV 过大会导致手臂模型显示异常（手臂可能穿模）。
- 默认 FOV 为 55 或许是因为手臂模型本身较短。

## Wallpaper Engine 动态壁纸导出图片

右键壁纸 -> `Play in Window` -> `Full HD Preview`，然后对 `Wallpaper Pop-out` 先放大，后截屏。

```powershell
.\scripts\Capture-Window-4K.ps1                              # 默认找 "Wallpaper Pop-out" 截 3840x2160
.\scripts\Capture-Window-4K.ps1 -Width 1920 -Height 1080     # 换分辨率
.\scripts\Capture-Window-4K.ps1 -OutFile "$env:USERPROFILE\Desktop\wall_4k.png"
.\scripts\Capture-Window-4K.ps1 -ListWindows                 # 列出所有窗口标题，便于排查
```