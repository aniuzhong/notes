# CentOS Stream 8 杂记

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
