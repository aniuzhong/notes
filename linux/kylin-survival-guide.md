# 银河麒麟 (V10)

- [银河麒麟 (V10)](#银河麒麟-v10)
  - [系统参数](#系统参数)
  - [**重要踩坑经验**](#重要踩坑经验)
    - [**锁源** —— 麒麟软件商店会偷改 apt 源](#锁源--麒麟软件商店会偷改-apt-源)
    - [系统崩溃复盘 (重要教训)](#系统崩溃复盘-重要教训)
  - [可安全从系统软件源安装的软件包](#可安全从系统软件源安装的软件包)
  - [NVIDIA 闭源驱动 + CUDA 安装经验总结](#nvidia-闭源驱动--cuda-安装经验总结)
  - [使用 N 卡调用 VAAPI 进行视频硬件加速](#使用-n-卡调用-vaapi-进行视频硬件加速)
  - [软件支持情况](#软件支持情况)
    - [Flatpak](#flatpak)
    - [Firefox](#firefox)
      - [问题1](#问题1)
    - [Chrome](#chrome)
      - [桌面集成（让 Chrome 出现在菜单/默认应用，并接管网页链接）](#桌面集成让-chrome-出现在菜单默认应用并接管网页链接)
    - [VS Code](#vs-code)
    - [PyCharm](#pycharm)
    - [v2rayN](#v2rayn)
    - [企业微信 (不支持)](#企业微信-不支持)
    - [个人微信](#个人微信)
  - [编译安装 GCC 12.2.0](#编译安装-gcc-1220)
    - [编译安装 gmp-6.2.1](#编译安装-gmp-621)
    - [编译安装 isl-0.24](#编译安装-isl-024)
    - [编译安装 mpfr-4.1.0](#编译安装-mpfr-410)
    - [编译安装 mpc-1.2.1](#编译安装-mpc-121)
    - [编译安装 gcc-12.2.0](#编译安装-gcc-1220-1)
    - [使用 gcc-12.2.0](#使用-gcc-1220)
  - [AI](#ai)

未激活 ≠ 功能阉割。现状继续用完全没问题。

## 系统参数

- **Kylin V10 SP1 的 glibc 是 2.31**
- 内核 5.10.0-8-generic
- 窗口管理器 [ukui-kwin_x11](https://gitcode.com/openkylin/ukui-kwin)，EWMH 名 "KWin"

## **重要踩坑经验**

1. 装机时需要断网，不插 N 卡
2. 不能使用 `sudo apt update`
3. 不能使用 `sudo apt upgrade`
4. 进入系统后锁源
5. 尽量少地使用 `sudo apt install`

### **锁源** —— 麒麟软件商店会偷改 apt 源

- 元凶:`kylin-software-center-plugin-synchrodata` 的用户级定时器 **kylin-software-center.timer**,登录 1 分钟后从服务器同步数据(往源里塞 bugfix-limit 等仓库)

```shell
# 断网、拔 N 卡，安装完成后
# （可选）恢复模式 root shell，或进图形后 1 分钟内:
sudo systemctl mask kylin-source-update.service kylin-system-updater.service kylin-unattended-upgrades.service
sudo mv /usr/bin/kylin-source-update /usr/bin/kylin-source-update.bak
sudo chattr +i /etc/apt/sources.list /etc/apt/sources.list.d
```

### 系统崩溃复盘 (重要教训)

- 曾用 **docker 26.1.3**(ESM 源版)且 `systemctl enable docker` 开机自启
- 表现:网络断 → 重启 → 进不了图形界面
- 原因推断:dockerd 开机自启时重建网桥 + 操作 iptables/nftables,在 5.10 老内核上搅乱网络栈,波及桌面登录链路
- **对策:docker 全家( docker.service / docker.socket / containerd.service)全部 systemctl disable,不开机自启**
- 附带认知:麒麟基础仓库的 **docker.io 20.10.7 与 5.10 内核年代匹配**，26.1.3 版本是错配（见**锁源**）

## 可安全从系统软件源安装的软件包

- 字体包

```bash
sudo apt install fonts-noto-cjk fonts-noto-cjk-extra fonts-noto-color-emoji fonts-symbola fonts-hanazono
```

- 工具

```bash
sudo apt install ranger electerm git gdb cmake build-essential vim-gtk3 xdotools
```

- 开发 NPE

```bash
sudo apt install qtbase5-dev libxinerama-dev libxfixes-dev libxmu-dev libasound2-dev libpulse-dev
```

- 开发 LibreOffice

```bash
sudo apt install automake m4 ccache nasm graphviz python3-dev libcups2-dev libfontconfig1-dev gperf doxygen libxslt1-dev xsltproc libxml2-utils libxrandr-dev bison flex libgstreamer-plugins-base1.0-dev libgstreamer1.0-dev ant ant-optional libnss3-dev libavahi-client-dev libxt-dev libcairo2-dev libx11-xcb-dev
```

## NVIDIA 闭源驱动 + CUDA 安装经验总结

> 机器：HP Z2 工作站，Intel UHD 630（核显）+ NVIDIA Quadro RTX 4000（Turing, 10DE:1EB1）

1. 装驱动前 BIOS 打开集显，插入 NVIDIA 显卡。
2. 装驱动时可用用集显亮屏，安装 NVIDIA 驱动。
3. 验证驱动可用后，关闭集显（避免奇怪错误）。

经过验证的驱动版本:

- cuda_12.8.1_570.124.06_linux.run

## 使用 N 卡调用 VAAPI 进行视频硬件加速

以下条件均需要正确满足

- VA-API 标准接口
- libva VA-API 的实现库
- NVIDIA 驱动
- nvidia-vaapi-driver 开源软件，在 Linux 系统中为 NVIDIA GPU 提供 VA-API 接口支持

参考 ArchWiki，使用 mpv 检测当前系统的 vaapi 是否可用。

```Bash
mpv --vo=gpu --hwdec=vaapi /path/to/video_file
```

> 从实践经验来看，优先参考 Ubuntu 发行版软件仓库的版本配对。

**Ubuntu 24.04**

| Driver Version | CUDA Version | nvidia-vaapi-driver | VA-API | libva  |
| -------------- | ------------ | ------------------- | ------ | ------ |
| 550.120        | 12.4         | v0.0.8              | 1.20   | 2.12.0 |

**Ubuntu 24.10**

| Driver Version | CUDA Version | nvidia-vaapi-driver | VA-API | libva  |
| -------------- | ------------ | ------------------- | ------ | ------ |
| 560.35.03      | 12.6         | v0.0.12             | 1.22   | 2.22.0 |

## 软件支持情况

### Flatpak

```bash
# 装新版 flatpak（系统 1.6.5 太老，连不上 Flathub 摘要）
#    从 flatpak PPA 手工 dpkg（不碰 apt 源），本机 glibc 2.31 需用 1.14.10 的 18.04 构建
sudo dpkg -i \
  bubblewrap_0.11.0-2~flatpak1~20.04.1_amd64.deb \
  libostree-1-1_2020.8-flatpak2~20.04_amd64.deb \
  libappstream4_0.12.10-2_amd64.deb \
  flatpak_1.14.10-1~flatpak2~18.04.1_amd64.deb
```

目前实测可用的 Flatpak 源软件：

- [x] Chrome
- [x] Steam
- [x] WPS 365
- [x] Podman Desktop

### Firefox

[支持](https://www.firefox.com/thanks/)。

#### 问题1

Firefox 用的是 GTK 客户端装饰(CSD)——窗口边框和缩放边缘由它自己画，而不是窗口管理器。在 UKUI 的窗口管理器 ukui-kwin（KWin） 下，Firefox 的 CSD 缩放边缘配合失灵，所以怎么拖边缘都没反应。

修复方式：让 Firefox 改用系统原生标题栏，把窗口框架和缩放边框交给 KWin 绘制。

### Chrome

> 在 Kylin（NVIDIA+UKUI）上跑**官方 Chrome + 登录 Google 账号 + 书签同步**。结论是**官方 deb 灰屏、CfT 不能同步，只能走 flatpak `com.google.Chrome`**。

1. 用 Flatpak 安装

    ```bash
    # 加 Flathub 远程 + 装官方 Chrome（含 N 卡 Vulkan GL 运行时，约几百 MB）
    flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
    flatpak install --user -y flathub com.google.Chrome
    # 运行
    flatpak run com.google.Chrome
    ```

2. 配置 XDG_DATA_DIRS（让 Chrome 出现在菜单/应用列表）

3. 创建启动器 ~/.local/share/applications/google-chrome.desktop

    > Exec 行必须带 --file-forwarding @@u %U @@，否则 xdg-open/VSCode 传来的 URL 不会进入 flatpak 沙箱：

4. 设为默认浏览器
    ```bash
    xdg-settings set default-web-browser google-chrome.desktop
    xdg-mime default google-chrome.desktop x-scheme-handler/http x-scheme-handler/https
    ```
#### 桌面集成（让 Chrome 出现在菜单/默认应用，并接管网页链接）

```bash
# 让 flatpak 应用可见：持久化 XDG_DATA_DIRS
# 已写入 ~/.config/environment.d/flatpak.conf

# 在控制中心可选的启动器（标准目录）
# 已建 ~/.local/share/applications/google-chrome.desktop

# 设为默认浏览器
xdg-settings set default-web-browser google-chrome.desktop
xdg-mime default google-chrome.desktop x-scheme-handler/http x-scheme-handler/https text/html
```

### VS Code

可安装版本：

- code_1.134.0-1787078834_amd64.deb

### PyCharm

- flatpak 可装

### v2rayN

当前最新版 7.21.2 下载 `v2rayN-linux-64.zip` 可运行，`deb` 版本安装不上。

### 企业微信 (不支持)

不支持，已尝试过若干办法。

### 个人微信

支持，[下载地址](https://linux.weixin.qq.com/en)

## 编译安装 GCC 12.2.0

> Kylin V10 自带 GCC 编译器不支持 C++20，需要手动编译高版本 GCC

### 编译安装 gmp-6.2.1

``` shell
wget ftp://gcc.gnu.org/pub/gcc/infrastructure/gmp-6.2.1.tar.bz2
tar -jxf gmp-6.2.1.tar.bz2
cd gmp-6.2.1
./configure --prefix=/opt/local/gmp-6.2.1
make -j8
make install
```

### 编译安装 isl-0.24

``` shell
wget ftp://gcc.gnu.org/pub/gcc/infrastructure/isl-0.24.tar.bz2
tar -jxf isl-0.24.tar.bz2
cd isl-0.24
./configure --prefix=/opt/local/isl-0.24 --with-gmp-prefix=/opt/local/gmp-6.2.1
make -j8
make install
```

### 编译安装 mpfr-4.1.0

``` shell
wget ftp://gcc.gnu.org/pub/gcc/infrastructure/mpfr-4.1.0.tar.bz2
tar -jxf mpfr-4.1.0.tar.bz2
cd mpfr-4.1.0
./configure --prefix=/opt/local/mpfr-4.1.0 --with-gmp=/opt/local/gmp-6.2.1
make -j8
make install
```

### 编译安装 mpc-1.2.1

``` shell
wget ftp://gcc.gnu.org/pub/gcc/infrastructure/mpc-1.2.1.tar.gz
tar -zxf mpc-1.2.1.tar.gz
cd mpc-1.2.1
./configure --prefix=/opt/local/mpc-1.2.1 --with-gmp=/opt/local/gmp-6.2.1 --with-mpfr=/opt/local/mpfr-4.1.0
make -j8
make install
```

### 编译安装 gcc-12.2.0

``` shell
wget https://ftp.gnu.org/gnu/gcc/gcc-12.2.0/gcc-12.2.0.tar.gz
export LD_LIBRARY_PATH=/opt/local/gmp-6.2.1/lib:/opt/local/mpfr-4.1.0/lib:/opt/local/mpc-1.2.1/lib:/opt/local/isl-0.24/lib:$LD_LIBRARY_PATH
tar -zxf gcc-12.2.0.tar.gz
cd gcc-12.2.0
./configure --prefix=/opt/local/gcc-12.2.0 --with-gmp=/opt/local/gmp-6.2.1 --with-mpfr=/opt/local/mpfr-4.1.0 --with-mpc=/opt/local/mpc-1.2.1 --with-isl=/opt/local/isl-0.24 --disable-multilib
make -j8
make install
```

### 使用 gcc-12.2.0

``` shell
export LD_LIBRARY_PATH=/opt/local/gmp-6.2.1/lib:/opt/local/mpfr-4.1.0/lib:/opt/local/mpc-1.2.1/lib:/opt/local/isl-0.24/lib:$LD_LIBRARY_PATH
export CC=/opt/local/gcc-12.2.0/bin/gcc
export CXX=/opt/local/gcc-12.2.0/bin/g++
```

## AI

- [x] Claude Code
- [ ] ChatGPT (Codex)
- [x] ZCode
- [x] DeepSeek Harness
- [x] Trae CN
- [x] CodeBuddy
- [x] Devin
- [x] Qoder
- [x] OpenCode