# 银河麒麟 (V10)

- [银河麒麟 (V10)](#银河麒麟-v10)
  - [**重要踩坑经验**](#重要踩坑经验)
  - [**锁源** —— 麒麟软件商店会偷改 apt 源](#锁源--麒麟软件商店会偷改-apt-源)
  - [可以安全从系统软件源安装的软件包](#可以安全从系统软件源安装的软件包)
  - [软件支持情况](#软件支持情况)
    - [v2rayN 支持情况](#v2rayn-支持情况)
    - [企业微信 (不支持)](#企业微信-不支持)
    - [个人微信 (支持)](#个人微信-支持)
  - [AI 支持情况](#ai-支持情况)

未激活 ≠ 功能阉割。现状继续用完全没问题。

## **重要踩坑经验**

1. 装机时需要断网（最稳妥）
2. 不能使用 `sudo apt update`
3. 不能使用 `sudo apt upgrade`
4. 进入系统后锁源
5. 尽量少地使用 `sudo apt install`

## **锁源** —— 麒麟软件商店会偷改 apt 源

- 元凶:`kylin-software-center-plugin-synchrodata` 的用户级定时器 **kylin-software-center.timer**,登录 1 分钟后从服务器同步数据(往源里塞 bugfix-limit 等仓库)
- 处置:
  1. 删除 root 级自启链接:`sudo rm /etc/systemd/user/default.target.wants/kylin-software-center.timer`
  2. **apt 源上锁**(chattr +i,商店再怎么写都写不进去):
     ```bash
     sudo chattr +i /etc/apt/sources.list /etc/apt/sources.list.d
     ```
  - ⚠️ 以后手动加源要先解锁:`sudo chattr -i /etc/apt/sources.list /etc/apt/sources.list.d`

## 可以安全从系统软件源安装的软件包

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

## 软件支持情况

### v2rayN 支持情况

当前最新版 7.21.2 下载 `v2rayN-linux-64.zip` 可运行，`deb` 版本安装不上。

### 企业微信 (不支持)

### 个人微信 (支持)

[下载地址](https://linux.weixin.qq.com/en)

## AI 支持情况

- [x] Claude Code
- [ ] ChatGPT (Codex)
- [x] ZCode
- [x] DeepSeek Harness
- [x] Trae CN
- [x] CodeBuddy
- [x] Devin
- [x] Qoder