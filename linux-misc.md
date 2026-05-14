# Linux 杂记

- [Linux 杂记](#linux-杂记)
  - [**Ubuntu 26.04 LTS**](#ubuntu-2604-lts)
    - [默认中文输入法](#默认中文输入法)
    - [关闭自动熄屏](#关闭自动熄屏)
    - [Claude Code - DeepSeek - CC Switch](#claude-code---deepseek---cc-switch)
  - [**Kylin v10 (SP1)**](#kylin-v10-sp1)
    - [v2rayN 支持情况](#v2rayn-支持情况)
    - [Claude Code 支持情况](#claude-code-支持情况)
    - [CC Switch 支持情况](#cc-switch-支持情况)
  - [WSL2](#wsl2)
  - [查看某一内核版本是否支持某硬件](#查看某一内核版本是否支持某硬件)
  - [在 Linux 某一发行版上使用 N 卡调用 VAAPI 进行视频硬件加速](#在-linux-某一发行版上使用-n-卡调用-vaapi-进行视频硬件加速)
  - [CentOS Stream 8 BaseOS ISO 镜像地址 (x86\_64)](#centos-stream-8-baseos-iso-镜像地址-x86_64)
    - [CentOS Linux 8 生命周期结束继续使用方法](#centos-linux-8-生命周期结束继续使用方法)
  - [编译安装 GCC 12.2.0](#编译安装-gcc-1220)
    - [编译安装 gmp-6.2.1](#编译安装-gmp-621)
    - [编译安装 isl-0.24](#编译安装-isl-024)
    - [编译安装 mpfr-4.1.0](#编译安装-mpfr-410)
    - [编译安装 mpc-1.2.1](#编译安装-mpc-121)
    - [编译安装 gcc-12.2.0](#编译安装-gcc-1220-1)
    - [使用 gcc-12.2.0](#使用-gcc-1220)

## **Ubuntu 26.04 LTS**

现代 Linux 桌面的核心基础设施已发生变革：

- 显示服务从 X11 向 Wayland 迁移
- 音视频服务已基本统一到 PipeWire 框架之下

### 默认中文输入法

`Settings` > `Region & Language` > `Manage Installed Languages` > `Install / Remove Languages`

**重启**。

`Settings` > `Keyboard` > `Input Sources` > `Add Input Source` > **`Chinese (Intelligent Pinyin)`**

### 关闭自动熄屏

`Settings` > `Power` > `Power Savingj` > `Automatic Screen Blank`

### Claude Code - DeepSeek - CC Switch

```bash
curl -fsSL https://claude.ai/install.sh | bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc && source ~/.bashrc
```

下载 [CC Switch](https://github.com/farion1231/cc-switch/releases)

```bash
sudo apt install ./CC-Switch-v3.14.1-Linux-x86_64.deb
```

添加 DeepSeek

`(+)` > `DeepSeek`

- Main Model: deepseek-v4-pro[1m]
- Default Haiku Model: deepseek-v4-flash
- Default Sonnet Model: deepseek-v4-pro[1m]
- Default Oput Model: deepseek-v4-pro[1m]

运行，测试。

## **Kylin v10 (SP1)**

银河麒麟桌面操作系统 V10 SP1

### v2rayN 支持情况

当前最新版 7.21.2 下载 `v2rayN-linux-64.zip` 可运行，`deb` 版本安装不上。

### Claude Code 支持情况

截至目前，官方脚本可安装。

### CC Switch 支持情况

不能安装。只能手写环境变量。

```bash
export ANTHROPIC_BASE_URL="https://api.deepseek.com/anthropic"
export ANTHROPIC_AUTH_TOKEN=""
export ANTHROPIC_MODEL="deepseek-v4-pro[1m]"
export ANTHROPIC_DEFAULT_HAIKU_MODEL="deepseek-v4-flash"
export ANTHROPIC_DEFAULT_SONNET_MODEL="deepseek-v4-pro[1m]"
export ANTHROPIC_DEFAULT_OPUS_MODEL="deepseek-v4-pro[1m]"
```

## WSL2

WSL2 不支持 VAAPI 视频硬件解码：

- 无 /dev/dri/renderD128
- GPU 驱动是 dxgkrnl 而非 i915

CUDA 硬解纹理只能在 CUDA 内部做处理（转换、缩放等都行），唯独显示前需要 cudaMemcpy 到 CPU，因为

- GPU 显示端：Mesa D3D12 -> Intel UHD Graphics 770 (iGPU 硬件)
- GPU 计算端：CUDA

两边的厂商、驱动、GPU 完全不同。WSLg 把显示交给 Mesa（Intel iGPU），CUDA 跑在 NVIDIA 独显上。

> 原生 Linux（同一家驱动）。

## 查看某一内核版本是否支持某硬件

[Hardware for Linux](https://linux-hardware.org)

点 `Parts`，通过 `lspci` 命令获取本机设备 `Vendor` 以及 `Name` 来查看某一内核版本是否支持该设备。

> 注：集显没有驱动在 Linux 下会退化为 [LLVMpipe](https://docs.mesa3d.org/drivers/llvmpipe.html)

## 在 Linux 某一发行版上使用 N 卡调用 VAAPI 进行视频硬件加速

以下条件均需要正确满足

- VA-API 标准接口
- libva VA-API 的实现库
- NVIDIA 驱动
- nvidia-vaapi-driver 开源软件，在 Linux 系统中为 NVIDIA GPU 提供 VA-API 接口支持

参考 ArchWiki，使用 mpv 检测当前系统的 vaapi 是否可用。

```Bash
mpv --vo=gpu --hwdec=vaapi /path/to/video_file
```

> 从实践经验来看，配置其他发行版时优先参考 Ubuntu 发行版软件仓库的版本配对。

**Ubuntu 24.04**

| Driver Version | CUDA Version | nvidia-vaapi-driver | VA-API | libva  |
| -------------- | ------------ | ------------------- | ------ | ------ |
| 550.120        | 12.4         | v0.0.8              | 1.20   | 2.12.0 |

**Ubuntu 24.10**

| Driver Version | CUDA Version | nvidia-vaapi-driver | VA-API | libva  |
| -------------- | ------------ | ------------------- | ------ | ------ |
| 560.35.03      | 12.6         | v0.0.12             | 1.22   | 2.22.0 |

## CentOS Stream 8 BaseOS ISO [镜像地址](https://compose-02.aws.centos.org/latest-CentOS-Stream-8/compose/BaseOS/x86_64/iso) (x86_64)

> 📌 建议：下载完整安装镜像（包含全部软件包）**CentOS-Stream-8-x86_64-20230209-dvd1.iso**  

### CentOS Linux 8 生命周期结束继续使用方法

- **停止维护日期**：CentOS Linux 8 已于 **2021 年 12 月 31 日**正式到达生命周期终点 (EOL)。
- **镜像移除时间**：官方在 **2022 年 1 月 31 日**之后，将 CentOS 8 的软件包从主镜像 (`mirror.centos.org`) 移除。
- **归档位置**：所有 CentOS 8 的软件包已永久迁移至 **[vault.centos.org](http://vault.centos.org)**。
- **继续使用方法**：
  1. 编辑 `/etc/yum.repos.d/*.repo` 文件。
  2. 将 `mirror.centos.org` 替换为 `vault.centos.org`。
  3. 示例（需要管理员权限）：
     ```
     sed -i -e "s|mirrorlist=|#mirrorlist=|g" /etc/yum.repos.d/CentOS-*
     sed -i -e "s|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g" /etc/yum.repos.d/CentOS-*
     dnf update
     dnf upgrade
     ```


## 编译安装 GCC 12.2.0

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