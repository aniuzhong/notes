# WSL2

- [WSL2](#wsl2)
  - [启用 WSL 和虚拟机平台](#启用-wsl-和虚拟机平台)
  - [GUI](#gui)

## 启用 WSL 和虚拟机平台

```shell
# 使用 dism（避免 WMI 错误）
dism /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
dism /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart
```

## GUI

下载中日韩字体

```bash
sudo apt install fonts-noto-cjk fonts-noto-color-emoji
```

强制使用独显加速

```bash
echo 'export GALLIUM_DRIVER=d3d12' | sudo tee /etc/profile.d/wslg-gpu.sh
```

WSL2 不支持 VAAPI 视频硬件解码：

- 无 /dev/dri/renderD128
- GPU 驱动是 dxgkrnl 而非 i915

CUDA 硬解纹理只能在 CUDA 内部做处理（转换、缩放等都行），唯独显示前需要 cudaMemcpy 到 CPU，因为

- GPU 显示端：Mesa D3D12 -> Intel UHD Graphics 770 (iGPU 硬件)
- GPU 计算端：CUDA

两边的厂商、驱动、GPU 完全不同。WSLg 把显示交给 Mesa（Intel iGPU），CUDA 跑在 NVIDIA 独显上。

> 原生 Linux（同一家驱动）。
