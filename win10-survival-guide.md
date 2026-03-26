# Windows 10 Survival Guide

- [Windows 10 Survival Guide](#windows-10-survival-guide)
  - [🔐 Activate Windows 10 Professional](#-activate-windows-10-professional)
  - [⚙️ Windows Settings](#️-windows-settings)
    - [System Locale](#system-locale)
    - [🔋 Ultimate Performance Power Plan](#-ultimate-performance-power-plan)
    - [Visual Effects](#visual-effects)
    - [Quick access](#quick-access)
    - [Personalize](#personalize)
      - [Start](#start)
    - [Windows 默认应用设置（Default apps）](#windows-默认应用设置default-apps)
    - [App Execution Aliases](#app-execution-aliases)
    - [连按 5 次 Shift 开启粘滞键](#连按-5-次-shift-开启粘滞键)
  - [⛑️ Sandboxie-Plus](#️-sandboxie-plus)
    - [需要隔离运行的软件](#需要隔离运行的软件)
  - [⛑️ Window Sandbox](#️-window-sandbox)
  - [🗑️ Uninstall Tool](#️-uninstall-tool)
    - [Pre-installed apps (that safe to uninstall)](#pre-installed-apps-that-safe-to-uninstall)
  - [🛒 Microsoft Store](#-microsoft-store)
    - [Microsoft Store - Generation Project](#microsoft-store---generation-project)
    - [HEVC Video Extensions from Device Manufacturer](#hevc-video-extensions-from-device-manufacturer)
    - [HDR + WCG Image Viewer](#hdr--wcg-image-viewer)
  - [🔎 Listary v6.2.0.42](#-listary-v62042)
    - [Upgrade to Listary Pro](#upgrade-to-listary-pro)
    - [Options](#options)
  - [🗃️ Bandizip](#️-bandizip)
  - [FastStone Capture 11.2](#faststone-capture-112)
  - [🏄 Chrome](#-chrome)
    - [Remove-MS-Edge](#remove-ms-edge)
  - [📫 Outlook](#-outlook)
  - [🔧 Hardware Management](#-hardware-management)
  - [Others](#others)

## 🔐 Activate Windows 10 Professional

Press <kbd>Win</kbd> + <kbd>X</kbd>, then click **Windows PowerShell (Admin)**.

```powershell
slmgr /ipk W269N-WFGWX-YVC9B-4J6C9-T83GX
slmgr /skms kms.03k.org
slmgr /ato
```

## ⚙️ Windows Settings

### System Locale

<kbd>Win</kbd> + <kbd>R</kbd> → `control intl.cpl`

### 🔋 Ultimate Performance Power Plan

Press <kbd>Win</kbd> + <kbd>X</kbd>, then click **Windows PowerShell (Admin)**.

```powershell
powercfg.exe -duplicatescheme e9a42b02-d5df-448d-aa00-03f14749eb61
```

This command creates a copy of the hidden "Ultimate Performance" power plan on we can select it in Power Options.

<kbd>Win</kbd> + <kbd>R</kbd> → `control powercfg.cpl`

### Visual Effects

<kbd>Win</kbd> + <kbd>R</kbd> → `SystemPropertiesPerformance`

在 **性能设置** - **视觉效果** 中打开/关闭动画效果

### Quick access

<kbd>Win</kbd> + <kbd>R</kbd> → `control folders`

- ❌ Show recently used files in Quick access

- ❌ Show frequently used folders in Quick access

### Personalize

<kbd>Win</kbd> + <kbd>I</kbd> → `Personalization`

#### Start

- ❌ Show recently added apps
- ❌ Show most used apps
- ❌ Show suggestions occasionally in Start

### Windows 默认应用设置（Default apps）

<kbd>Win</kbd> + <kbd>R</kbd> → `ms-settings:defaultapps`

### App Execution Aliases

### 连按 5 次 Shift 开启粘滞键

<kbd>Win</kbd> + <kbd>I</kbd> > `Ease of Access` > `Keyboard` 

- ❌ `Allow the shortcut key to start Sticy Keys`

## ⛑️ Sandboxie-Plus

`选项` -> `全局设置` -> `常规选项` -> `消息通知` -> `将可恢复的文件以通知形式显示`

### 需要隔离运行的软件

- 微信 (保护个人信息)
- 网易邮箱 (保护个人信息)
- WPS
- 夸克网盘
- 百度网盘
- 迅雷
- 2345好压
- 2345看图王
- 360压缩

## ⛑️ Window Sandbox

火中取栗。

```powershell
Enable-WindowsOptionalFeature -FeatureName "Containers-DisposableClientVM" -All -Online
```

## 🗑️ Uninstall Tool

> Uninstall Tool 的免费版为 Geek Uninstaller

### Pre-installed apps (that safe to uninstall)

- 3D Viewer
- Clock & Clock Widget
- Camera
- Copilot
- Calculator (**保留**)
- Calendar
- Cortana
- Feedback Hub
- Groove Music
- Game Bar
- Get Helps
- Mail
- Maps
- Movies & TV
- Media Player
- OneNote
- Outlook (**可保留**)
- Pay
- People
- Paint 3D
- Photos (**保留**)
- Phone Link
- Sticky Notes
- Snip & Sketch
- Skype
- Solitaire & Casual Games
- Tips
- Mixed Reality Portal
- Voice Recorder
- Weather
- Xbox Console Companion / Game Bar / Xbox Live

## 🛒 Microsoft Store

> 🚨 请备份应用安装包，商店下架后将无法再获取。

### Microsoft Store - Generation Project

一个非官方的 [Microsoft Store 链接生成器](https://store.rg-adguard.net)

### HEVC Video Extensions from Device Manufacturer

使 Microsoft Photos 支持 HEIC 格式的图片

- URL: https://apps.microsoft.com/detail/9N4WGH0Z6VHQ
- ProductID: 9N4WGH0Z6VHQ

### [HDR + WCG Image Viewer](https://github.com/13thsymphony/HDRImageViewer)

- URL: https://apps.microsoft.com/detail/9pgn3nwpbwl9
- ProductID: 9pgn3nwpbwl9

## 🔎 Listary v6.2.0.42

### Upgrade to Listary Pro

- Email: samoray@samoray.com
- Registration code:

```
JE4V8T3M96PWT4SUCNZNVZ37XKLBU2QW
N64LEJQ3VHY7MPL6KY2R5SQZ76QFFTKP
VVE8JBVEAME8MMBRHRGF2P6MAJG7ZSQY
MUY2PGTQ2EG3W2YHTU2CFWL7SE6THU3Q
TAL3U894S2BWA9629EFCXYYYG3S65WEQ
N3SZSNSEGXMEUWD27PNENP4GR2EKKDK6
```

### Options

- `General` -> Share anonymous analytics data to help improve Listary ❌
- `General` -> Check for updates automatically ❌

## 🗃️ Bandizip

> Bandizip **v6.29** is the last **free** release without ads.

> Delete `C:\Program Files\Bandizip\Updater.exe`

## FastStone Capture 11.2

- User Name: bluman
- Registration code: VPISCJULXUFGDDXYAUYF

## 🏄 Chrome

### [Remove-MS-Edge](https://github.com/ShadowWhisperer/Remove-MS-Edge)

> Do a fresh install of Windows 10. Fully update the system, then run Remove-Edge.exe  — everything should work fine.

## 📫 Outlook

Windows 默认邮箱客户端，可用来登录 gmail。

## 🔧 Hardware Management

- Fan Control
- FurMark
- HWMonitor
- AIDA64 Extreme v8.20.8100

```
Y6F3R-C1HD6-2TDJB-HDSZY-F9T3M
4H514-EYYD6-1MDKP-SDA7Y-UKQSG
FPPSY-VUTD6-56DJX-2D2KY-WD2UW
```

## Others

> 微软拼音输入法在**输入时**，按 <kbd>Shift</kbd> + <kbd>Space</kbd> 会切换从`半角`切换为`全角`，再次按 <kbd>Shift</kbd> + <kbd>Space</kbd> 便能切换回去。