# 网络知识杂记

- [网络知识杂记](#网络知识杂记)
  - [Proxy Clients](#proxy-clients)
    - [名词](#名词)
    - [示例](#示例)
    - [v2rayN](#v2rayn)
    - [v2rayN 的运行方式](#v2rayn-的运行方式)
    - [测速时提示“运行Core失败，请看日志”](#测速时提示运行core失败请看日志)
  - [Leaks Test](#leaks-test)
  - [为 WSL2 配置 Windows 代理](#为-wsl2-配置-windows-代理)

## Proxy Clients

### 名词

- 节点：单个服务器的配置。
- 订阅：节点的集合，通常以链接形式提供。
- 机场：提供订阅的服务商或平台，本质上是“节点供应商”。
- TUN 模式：是指通过在操作系统中创建一个**虚拟网卡**，来实现网络流量的接管。
- BGP/专线：描述的是入口和中转的质量。
- 原生/家宽：描述的是落地 IP 的类型。
- 流媒体解锁：意味着该落地 IP 专门优化过。

- **“找个好用的节点”**（强调单个服务器配置）
- **“找个靠谱的订阅”**（强调批量导入的链接）
- **“找个稳定的机场”**（强调提供服务的平台/商家）
- **“找个好用的机场”** -> 这是最普遍的说法，因为机场本身就包含了订阅和节点。
- **“换个机场”** -> 指的是更换服务商。
- **“机场跑路了”** -> 指某个服务商停止运营或失效。
- **“节点挂了”** -> 指单个服务器不可用。

### 示例

- `US03 | Premium | 1.5x`
- `JP 福利 0.3X`
- `香港一区[三线][1.0倍消耗]`
- `韩国[中转][1.0倍消耗]`
- `日本一区[直连][1.0倍消耗]`
- `新加坡二区[移动][1.0倍消耗]`
- `香港住宅[纯净家宽][3.0倍消耗]`
- `日本住宅[动态家宽][5.0倍消耗]`
- `英国原生[焕新原生][2.0倍消耗]`
- `香港二区[流媒体&游戏][1.0倍消耗]`

### [v2rayN](https://github.com/2dust/v2rayN)

- `设置` -> `以管理员身份重启`
- `配置项` -> `从剪贴板导入分享链接`
- `订阅分组` -> `更新当前订阅（不通过代理）`
- 右键某一节点`设为活动`

---

### v2rayN 的运行方式

v2rayN 是 Windows 平台上的图形化客户端，它调用 V2Ray 核心 来处理代理协议。

1. **本地应用 → v2rayN 服务**

    浏览器设置代理为 127.0.0.1:1080（SOCKS5），流量先进入 v2rayN。

2. **v2rayN → 远程节点**

    v2rayN 将流量加密，发送到配置的远程代理节点（例如美国服务器）

3. **节点 → 目标网站**

    节点访问目标网站（如 Google），目标网站只看到节点的 IP。

4. **返回链路**

    网站响应 → 节点 → v2rayN → 本地应用。

---

- **V4-绕过大陆 (Whitelist)**
    - 大陆网站（如国内常见的服务）不走代理，直接连接。
    - 只有在“白名单”里的国外网站才会通过代理访问。
    - 常用于需要访问少量被屏蔽的国外网站，同时保持国内网络速度。
- **V4-黑名单 (Blacklist)**
    - 默认所有网站都走代理。
    - 但“黑名单”里的特定网站（通常是国内网站）会绕过代理，直接访问。
    - 常用于需要稳定访问国外网站，但又希望国内网站保持直连。
- **V4-全局 (Global)**
    - 所有流量都走代理，没有例外。
    - 常用于需要完全隐藏真实网络环境，或访问国外服务为主的场景。
    - 缺点是访问国内网站可能会变慢。

### 测速时提示“运行Core失败，请看日志”

如果启动了 hyper-v 类的服务（例如 WSA，hyper-v 虚拟机或者 WSL），那应该是端口被 winnat 占用了。

使用命令：

```powershell
netsh int ip show excludedportrange protocol=tcp
```

查看 10808 端口是否在占用范围内。

如果在，则打开 `设置` > `参数设置` > `本地混合监听端口`，换成一个不在占用范围内的端口。

---

## Leaks Test

[Browser Leaks](https://browserleaks.com/ip)

[DNS Leak Test](https://dnsleaktest.com)

## 为 WSL2 配置 Windows 代理

- 系统版本：Windows 10 22H2

```
WSL2  →  Windows网卡(172.17.144.1):10101  →  127.0.0.1:10100(v2rayN)  →  互联网
         └── netsh portproxy 负责 ──┘          └── v2rayN 配置决定 ──┘
```

```powershell
# 管理员权限运行

# v2rayN 安装位置
$v2rayConfig = "C:\Users\hido\Softwares\v2rayN-7.17.3-x64-portable\binConfigs\config.json"

# 查 vEthernet (WSL) 这块虚拟网卡（Hyper-V 给 WSL2 创建的）的 IPv4 地址，
# 取到的值就是 172.17.144.1 这类东西，放进 $wslIp，
# 如果没拿到 IP（说明 WSL2 没在运行，虚拟网卡不存在），打印红色错误并退出。
$wslIp = (Get-NetIPAddress -InterfaceAlias "vEthernet (WSL)" -AddressFamily IPv4).IPAddress
if (-not $wslIp) {
    Write-Host "ERROR: WSL2 vEthernet not found. Is WSL running?" -ForegroundColor Red
    exit 1
}

# Get-Content -Raw 把整个 config.json 读成一个字符串，管道给 ConvertFrom-Json 解析成 PowerShell 对象。之后就能用 . 访问字段了。
$config = Get-Content $v2rayConfig -Raw | ConvertFrom-Json
$proxyPort = $config.inbounds[0].port
if (-not $proxyPort) {
    Write-Host "ERROR: Cannot read proxy port from v2rayN config" -ForegroundColor Red
    exit 1
}

# 在代理端口基础上 +1 作为转发监听端口，避免和 v2rayN 自己冲突。
$listenPort = $proxyPort + 1  # 10101

# 删掉之前可能存在的旧规则（如果 IP 变了，旧规则绑的是旧 IP，需要清理）。
# 2>$null 扔掉 stderr，Out-Null 扔掉 stdout，干净不刷屏。
netsh interface portproxy delete v4tov4 listenport=$listenPort 2>$null | Out-Null

# 建新规则：在 WSL2 虚拟网卡 IP 上监听 $listenPort，收到的 TCP 流量转发到 127.0.0.1:$proxyPort（v2rayN）。
netsh interface portproxy add v4tov4 listenaddress=$wslIp listenport=$listenPort connectaddress=127.0.0.1 connectport=$proxyPort

# 打印结果，比如 OK: 172.17.144.1:10101 -> 127.0.0.1:10100 (v2rayN)。`: 是 PowerShell 里冒号的转义写法。
Write-Host "OK: $wslIp`:$listenPort -> 127.0.0.1`:$proxyPort (v2rayN)" -ForegroundColor Green
```

在 WSL2 终端里：

```bash
vim ~/.bashrc
```

找到这两行：

```bash
export WIN_HOST=$(sed -n 's/nameserver //p' /etc/resolv.conf)    # 动态取网关IP
export http_proxy="http://$WIN_HOST:10101"                       # 用这个IP拼代理地址
export https_proxy="http://$WIN_HOST:10101"
export no_proxy="localhost,127.0.0.1,*.local,10.*,172.16.*,172.17.*,172.18.*,172.19.*,172.20.*,172.21.*,172.22.*,172.23.*,172.24.*,172.25.*,172.26.*,172.27.*,172.28.*,172.29.*,172.30.*,172.31.*,192.168.*"
```

验证：

```bash
curl -I https://www.google.com
```