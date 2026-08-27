#!/bin/bash
# 彻底修复 fontconfig 的 monospace 别名
# 用 match target="pattern" + binding="strong" 强制 monospace → DejaVu Sans Mono
# 这是 fontconfig 最高优先级手段，不受 Noto CJK 影响

set -e

TARGET="/etc/fonts/conf.avail/57-dejavu-sans-mono.conf"
LINK="/etc/fonts/conf.d/57-dejavu-sans-mono.conf"
BACKUP="${TARGET}.bak.$(date +%Y%m%d-%H%M%S)"

echo "=== 备份原文件 ==="
cp -v "$TARGET" "$BACKUP"
echo "备份到: $BACKUP"

echo ""
echo "=== 写入新的 57-dejavu-sans-mono.conf ==="
cat > "$TARGET" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE fontconfig SYSTEM "../fonts.dtd">
<fontconfig>
  <!-- 强制 monospace → DejaVu Sans Mono，优先级最高 -->
  <match target="pattern">
    <test name="family">
      <string>monospace</string>
    </test>
    <edit name="family" mode="prepend" binding="strong">
      <string>DejaVu Sans Mono</string>
    </edit>
  </match>

  <!-- 确保 DejaVu Sans Mono 的 fallback 包含 Noto CJK 以显示中文 -->
  <alias>
    <family>DejaVu Sans Mono</family>
    <default>
      <family>monospace</family>
    </default>
  </alias>
</fontconfig>
EOF

echo ""
echo "=== 刷新 fontconfig 缓存 ==="
fc-cache -f

echo ""
echo "=== 验证 monospace 字体解析 ==="
echo "首选:"
fc-match monospace
echo ""
echo "Fallback 链:"
fc-match -s monospace 2>/dev/null | head -8

echo ""
echo "=== 完成 ==="
echo "请重新打开 gvim 查看效果。"
echo "如需恢复，执行: sudo cp $BACKUP $TARGET && sudo fc-cache -f"