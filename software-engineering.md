# 软件工程

- [软件工程](#软件工程)
  - [**`The Mythical Man-Month - 人月神话`**](#the-mythical-man-month---人月神话)
    - [没有银弹 (No Silver Bullet)](#没有银弹-no-silver-bullet)
    - [本质困难 (essential difficulty) 和偶然困难 (accidental difficulty)](#本质困难-essential-difficulty-和偶然困难-accidental-difficulty)
  - [Vibe Coding (人+Agent)](#vibe-coding-人agent)
    - [传统抽象与代码生成](#传统抽象与代码生成)
  - [golden file / golden test / baseline test](#golden-file--golden-test--baseline-test)
  - [灰度测试](#灰度测试)
  - [MVP](#mvp)
  - [ROI](#roi)
  - [POC](#poc)
  - [绞杀者模式](#绞杀者模式)
  - [YAGHI](#yaghi)

## **`The Mythical Man-Month - 人月神话`**

### 没有银弹 (No Silver Bullet)

> 没有任何单一技术或管理方法能在十年内使软件生产力提升一个数量级。软件开发的根本困难在于概念性设计（思考做什么），而非实现（编码），后者可以通过工具改进，但前者难以被自动化。

### 本质困难 (essential difficulty) 和偶然困难 (accidental difficulty)

> 软件开发的困难分为本质困难和偶然困难。语法错误、环境配置、框架API——这些都是偶然困难，迟早能解决。但**把一个模糊的现实需求转化成精确的逻辑结构，这是本质困难，不会因为工具进步而消失**。

## Vibe Coding (人+Agent)

1. 人的时间和精力是有限的。
2. 一种技能如果长期不用，会退化；如果想让它一直保持熟练，就得持续练习。

> 现在看不到足够的理由去**长期支付这种维护成本**（某项特定技术），为了一个 **没法使用 Agent** 的场景。

### 传统抽象与代码生成

> 抽象代表了**一种定义问题的能力**，这在 Vibe Coding 中尤为重要。

`所有抽象都会泄漏 (All non-trivial abstractions, to some degree, are leaky)`, 所以，评估一个抽象的经验法则是问自己：**我需要多长时间 “窥探” 其底层实现一次？**

同样的，在使用 Coding Agent 工具进行代码生成时，也需要经常问自己：**我什么时候需要仔细检查 AI 到底写了什么？**

## golden file / golden test / baseline test

基准测试、黄金用例、夹具...

把**已知正确的一份输出**当作"**标准答案**"保存下来；之后程序一改，就跑同一套输入，把新生成的结果和这份标准答案做对比。若一致，就认为行为没被破坏。

**黄金用例**本身不证明业务上绝对正确，只证明和当初锁定的参考版本保持一致。

## 灰度测试

灰度测试（也叫灰度发布或金丝雀发布）就是：

> 先让一小部分用户先用新功能，没问题了再慢慢扩大到全部用户。

## MVP

MVP（Minimum Viable Product，最小可行产品）就是：

> 用最少的资源、最快的速度，先做出一个"能用的"版本。

## ROI

ROI 是 Return on Investment 的缩写。开发时，常说要优先做**高 ROI**的需求。

## POC

概念验证（Proof of Concept）, 指的是为了验证某个技术设想、算法或架构方案是否可行，而构建的一个小型、实验性的原型系统。

## 绞杀者模式

绞杀者模式（Strangler Pattern，也叫绞杀者模式或扼杀模式）就是：

> 不直接推翻旧系统重写，而是用新功能一点点蚕食、替换旧系统，最终旧系统自然"死亡"。

## YAGHI

> 别写你现在用不上的代码。