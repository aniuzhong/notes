<#
.SYNOPSIS
    将指定窗口（默认 "Wallpaper Pop-out"）先放大到 4K（2160p），再截取其客户区，
    输出 PNG 截图，截完后自动恢复窗口原大小/位置。

.DESCRIPTION
    原理：用 Win32 把窗口客户区调整到目标分辨率（3840x2160），
    再以 PrintWindow(PW_CLIENTONLY | PW_RENDERFULLCONTENT) 捕获——
    该标志会要求 DWM 渲染窗口的完整内容（含超出屏幕的部分），
    因此即使屏幕小于 4K 也能拿到 4K 客户区图像。

.PARAMETER WindowTitle
    要捕获的窗口标题（默认 "Wallpaper Pop-out"）。支持模糊匹配。

.PARAMETER Width / Height
    目标客户区分辨率（默认 3840x2160）。

.PARAMETER OutFile
    输出 PNG 路径（默认当前目录 Wallpaper_Pop-out_4K.png）。

.PARAMETER SettleMs
    放大后等待窗口重新渲染的毫秒数（默认 1500）。

.PARAMETER ListWindows
    只列出当前所有窗口标题，用于排查窗口真实标题。

.EXAMPLE
    .\Capture-Window-4K.ps1
.EXAMPLE
    .\Capture-Window-4K.ps1 -WindowTitle "Preview" -OutFile "$env:USERPROFILE\Desktop\wall_4k.png" -SettleMs 2000
.EXAMPLE
    .\Capture-Window-4K.ps1 -ListWindows
#>
[CmdletBinding()]
param(
    [string]$WindowTitle = "Wallpaper Pop-out",
    [int]$Width  = 3840,
    [int]$Height = 2160,
    [string]$OutFile = (Join-Path (Get-Location) "Wallpaper_Pop-out_4K.png"),
    [int]$SettleMs = 1500,
    [switch]$ListWindows
)

$ErrorActionPreference = 'Stop'

Add-Type -ReferencedAssemblies 'System.Drawing' -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Text;

public struct NRECT { public int Left, Top, Right, Bottom; }

public static class WinCap
{
    public delegate bool EnumProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr hWndParent, EnumProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder text, int count);
    [DllImport("user32.dll")]
    public static extern int GetWindowTextLength(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out NRECT rect);
    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr hWnd, out NRECT rect);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")]
    public static extern bool IsIconic(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint nFlags);
    [DllImport("user32.dll")]
    public static extern bool SetProcessDpiAwarenessContext(IntPtr value);

    // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = -4  →  坐标按物理像素
    public static void SetDpiAware() { try { SetProcessDpiAwarenessContext(new IntPtr(-4)); } catch { } }

    private static string TitleOf(IntPtr h)
    {
        int len = GetWindowTextLength(h);
        if (len <= 0 || len > 1024) return "";
        StringBuilder sb = new StringBuilder(len + 1);
        GetWindowText(h, sb, sb.Capacity);
        return sb.ToString();
    }

    private static void Collect(IntPtr root, List<KeyValuePair<IntPtr, string>> list)
    {
        string t = TitleOf(root);
        if (t.Length > 0) list.Add(new KeyValuePair<IntPtr, string>(root, t));
        EnumChildWindows(root, (h, l) => { Collect(h, list); return true; }, IntPtr.Zero);
    }

    public static string ListAllTitles()
    {
        var all = new List<KeyValuePair<IntPtr, string>>();
        EnumWindows((h, l) => { Collect(h, all); return true; }, IntPtr.Zero);
        var sb = new StringBuilder();
        foreach (var kv in all) sb.AppendLine(kv.Key + "  '" + kv.Value + "'");
        return sb.ToString();
    }

    public static IntPtr FindByTitle(string title)
    {
        IntPtr direct = FindWindow(null, title);
        if (direct != IntPtr.Zero) return direct;
        var all = new List<KeyValuePair<IntPtr, string>>();
        EnumWindows((h, l) => { Collect(h, all); return true; }, IntPtr.Zero);
        foreach (var kv in all)
            if (string.Equals(kv.Value, title, StringComparison.OrdinalIgnoreCase)) return kv.Key;
        foreach (var kv in all)
            if (kv.Value.IndexOf(title, StringComparison.OrdinalIgnoreCase) >= 0) return kv.Key;
        return IntPtr.Zero;
    }

    public static string Capture(IntPtr hwnd, int targetW, int targetH, string outFile, int settleMs)
    {
        if (hwnd == IntPtr.Zero) return "ERROR: window not found";

        NRECT origWin, origClient;
        if (!GetWindowRect(hwnd, out origWin)) return "ERROR: GetWindowRect failed";
        GetClientRect(hwnd, out origClient);

        int origW = origWin.Right - origWin.Left;
        int origH = origWin.Bottom - origWin.Top;
        int borderW = origW - origClient.Right;  if (borderW < 0) borderW = 0;
        int borderH = origH - origClient.Bottom; if (borderH < 0) borderH = 0;

        bool wasMin = IsIconic(hwnd);
        if (wasMin) ShowWindow(hwnd, 9); // SW_RESTORE

        try
        {
            // 1) 先放大：目标客户区 + 边框 = 新窗口尺寸，置于 (0,0) 保证左上角在屏内渲染。
            //    SWP_NOSENDCHANGING(0x0400) 会跳过 WM_WINDOWPOSCHANGING，从而绕过
            //    应用在 WM_GETMINMAXINFO 里设置的“最大尺寸=屏幕工作区”限制，
            //    让窗口能被放大到超过屏幕的真实 4K 尺寸。
            SetWindowPos(hwnd, IntPtr.Zero, 0, 0, targetW + borderW, targetH + borderH, 0x0650); // NOSENDCHANGING|NOZORDER|SHOWWINDOW|NOACTIVATE
            SetForegroundWindow(hwnd);
            System.Threading.Thread.Sleep(settleMs); // 等窗口按新分辨率渲染

            // 2) 再截图：按实际客户区大小创建位图，PrintWindow 渲染完整内容（含屏外部分）
            NRECT client; GetClientRect(hwnd, out client);
            int cw = client.Right, ch = client.Bottom;
            using (Bitmap bmp = new Bitmap(cw, ch, PixelFormat.Format32bppArgb))
            using (Graphics g = Graphics.FromImage(bmp))
            {
                IntPtr hdc = g.GetHdc();
                bool ok = PrintWindow(hwnd, hdc, 3u); // PW_CLIENTONLY(1) | PW_RENDERFULLCONTENT(2)
                g.ReleaseHdc(hdc);
                if (!ok) return "ERROR: PrintWindow returned false";
                bmp.Save(outFile, ImageFormat.Png);
            }
            return "OK  saved " + cw + "x" + ch + "  ->  " + outFile;
        }
        finally
        {
            // 3) 恢复窗口原位置与原大小
            SetWindowPos(hwnd, IntPtr.Zero, origWin.Left, origWin.Top, origW, origH, 0x0040);
        }
    }
}
'@

[WinCap]::SetDpiAware()

if ($ListWindows) {
    Write-Host "===== 当前所有窗口标题 =====" -ForegroundColor Cyan
    Write-Host ([WinCap]::ListAllTitles())
    exit 0
}

Write-Host "查找窗口: '$WindowTitle' ..." -ForegroundColor Cyan
$hwnd = [WinCap]::FindByTitle($WindowTitle)
if ($hwnd -eq [IntPtr]::Zero) {
    Write-Host "未找到标题为 '$WindowTitle' 的窗口。当前所有窗口标题：" -ForegroundColor Yellow
    Write-Host ([WinCap]::ListAllTitles())
    exit 1
}
Write-Host "已找到窗口句柄: $hwnd" -ForegroundColor Green

$result = [WinCap]::Capture($hwnd, $Width, $Height, $OutFile, $SettleMs)
if ($result -like "ERROR*") {
    Write-Host $result -ForegroundColor Red
    exit 1
}
Write-Host $result -ForegroundColor Green
