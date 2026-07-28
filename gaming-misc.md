# Gaming

- [Gaming](#gaming)
  - [EA 端口 3216 被占用问题解决方案](#ea-端口-3216-被占用问题解决方案)
    - [第一步：排查端口占用情况](#第一步排查端口占用情况)
    - [第二步：检查动态端口范围](#第二步检查动态端口范围)
    - [第三步：修改动态端口范围](#第三步修改动态端口范围)
  - [Steam 平台 `暴雨` Xbox 手柄右摇杆旋转不灵敏的解决方案](#steam-平台-暴雨-xbox-手柄右摇杆旋转不灵敏的解决方案)
  - [Steam 平台 `极乐迪斯科` Xbox 手柄没有切换语言键位的解决方法](#steam-平台-极乐迪斯科-xbox-手柄没有切换语言键位的解决方法)

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