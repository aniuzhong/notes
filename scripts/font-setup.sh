#!/usr/bin/env bash
# ============================================================================
# 字体一键部署脚本 —— Kylin V10 / Debian 系 / UKUI(MATE)
# 覆盖: 中英界面字体 + 彩色 emoji + 罕见汉字(扩展B区) + 符号兜底
#
# 用法(新机器上):
#   1. 手动执行 apt 安装(需要 sudo, 见下方 ① 节)
#   2. 运行本脚本(无需 sudo):  bash font-setup.sh
#
# 基于 2026-08-12 实际部署验证
# ============================================================================

set -euo pipefail

# ---------------------------------------------------------------------------
# ① apt 安装(需要 sudo —— 手动执行, 装完再跑本脚本)
#
#   sudo apt install -y fonts-noto-cjk fonts-noto-cjk-extra \
#       fonts-noto-color-emoji fonts-symbola fonts-hanazono
#
#   各包用途:
#   - fonts-noto-cjk            中文界面字体(部分发行版预装)
#   - fonts-noto-cjk-extra      罕见汉字字重补充
#   - fonts-noto-color-emoji    彩色 emoji
#   - fonts-symbola             杂项符号兜底
#   - fonts-hanazono            花园明朝 HanaMinA/B = 扩展B区罕见字全集
#   (Kylin V10 仓库实测全部可用)
#
#   可选: 若仓库里的 Noto CJK 过旧(如 2019 版), 可下载 2021 v2.004 放到
#   ~/.local/share/fonts/ 后手动 fc-cache -f(覆盖范围与新版相同, 仅版本差异):
#   https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf
#   https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Bold.otf
# ---------------------------------------------------------------------------

echo "==> [1/4] 写入 fontconfig 用户规则(彩色 emoji 优先)"
mkdir -p ~/.config/fontconfig
if [ -f ~/.config/fontconfig/fonts.conf ]; then
  cp ~/.config/fontconfig/fonts.conf ~/.config/fontconfig/fonts.conf.bak
  echo "    已有 fonts.conf, 已备份为 fonts.conf.bak"
fi
cat > ~/.config/fontconfig/fonts.conf << 'EOF'
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>
  <!-- 彩色 emoji 优先于 DejaVu 等单色字形 -->
  <alias>
    <family>sans-serif</family>
    <prefer><family>Noto Color Emoji</family></prefer>
  </alias>
  <alias>
    <family>serif</family>
    <prefer><family>Noto Color Emoji</family></prefer>
  </alias>
  <alias>
    <family>monospace</family>
    <prefer><family>Noto Color Emoji</family></prefer>
  </alias>
</fontconfig>
EOF
echo "    完成"

echo "==> [2/4] 对齐桌面界面字体设置(MATE + GNOME schema)"
if command -v gsettings >/dev/null 2>&1; then
  gsettings set org.mate.interface font-name 'Noto Sans CJK SC 10' 2>/dev/null \
    && echo "    org.mate.interface          -> Noto Sans CJK SC 10" \
    || echo "    (MATE schema 不存在, 跳过)"
  gsettings set org.gnome.desktop.interface font-name 'Noto Sans CJK SC 10' 2>/dev/null \
    && echo "    org.gnome.desktop.interface -> Noto Sans CJK SC 10" \
    || echo "    (GNOME schema 不存在, 跳过)"
  gsettings set org.gnome.desktop.interface monospace-font-name 'DejaVu Sans Mono 10' 2>/dev/null \
    && echo "    monospace                   -> DejaVu Sans Mono 10" || true
else
  echo "    (未找到 gsettings, 跳过)"
fi

echo "==> [3/4] 重建字体缓存"
fc-cache -f >/dev/null 2>&1
echo "    完成"

echo "==> [4/4] 验证覆盖(应均有输出)"
# 注: 不用 `cmd | head -1 || echo` 模式 —— pipefail 下 fc-list 大输出会因
#     head 提前退出被 SIGPIPE 杀掉(141), 误触发兜底分支。用变量 + || true。
line=$(fc-list ':charset=1f600' | grep -i 'color emoji' | head -1 || true)
echo "    彩色 emoji : ${line:-❌ 未找到!}"
line=$(fc-list ':charset=20000' | head -1 || true)
echo "    扩展B区罕见: ${line:-❌ 未找到!}"
line=$(fc-list ':charset=4e00' | head -1 || true)
echo "    中文基础   : ${line:-❌ 未找到!}"

cat << 'NOTE'

=====================================================================
部署完成。
已知遗留缺口(可忽略): U+9FFC-9FFF 与 U+4DB6-4DBF 共 14 个字符无任何
  字体覆盖, 如必须覆盖只能装 Unifont(位图):
  https://unifoundry.com/pub/unifont/unifont-15.1.05/font-builds/unifont-15.1.05.otf
验证字形覆盖请用 fc-list ':charset=XXXX' —— fc-match 带 charset 不可靠。
=====================================================================
NOTE
