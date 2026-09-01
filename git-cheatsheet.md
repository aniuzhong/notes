# Git 常用操作

- [Git 常用操作](#git-常用操作)
  - [git 局部用户名邮箱设置](#git-局部用户名邮箱设置)
  - [创建本地分支并设置追踪关系](#创建本地分支并设置追踪关系)

## git 局部用户名邮箱设置

```bash
git config user.name "aniuzhong"
git config user.email "zxy_9125@163.com"
git config --list --local
```

## 创建本地分支并设置追踪关系

```bash
git checkout --track origin/feature-x
```

一次性完成三件事：

- 创建本地分支 feature-x（如果不存在）
- 让它跟踪远程分支 origin/feature-x
- 切换到该分支