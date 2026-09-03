# Linux 杂记

- [Linux 杂记](#linux-杂记)
  - [`ld-linux.so` 搜索 `.so` 的顺序](#ld-linuxso-搜索-so-的顺序)
  - [查看某一内核版本是否支持某硬件](#查看某一内核版本是否支持某硬件)

## `ld-linux.so` 搜索 `.so` 的顺序

## 查看某一内核版本是否支持某硬件

[Hardware for Linux](https://linux-hardware.org)

点 `Parts`，通过 `lspci` 命令获取本机设备 `Vendor` 以及 `Name` 来查看某一内核版本是否支持该设备。

> 注：集显没有驱动在 Linux 下会退化为 [LLVMpipe](https://docs.mesa3d.org/drivers/llvmpipe.html)
