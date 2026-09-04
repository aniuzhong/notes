# 软件工程

- [软件工程](#软件工程)
  - [**`The Mythical Man-Month - 人月神话`**](#the-mythical-man-month---人月神话)
    - [没有银弹 (No Silver Bullet)](#没有银弹-no-silver-bullet)
  - [golden file / golden test / baseline test](#golden-file--golden-test--baseline-test)
  - [灰度测试](#灰度测试)
  - [MVP](#mvp)
  - [ROI](#roi)
  - [POC](#poc)
  - [绞杀者模式](#绞杀者模式)

## **`The Mythical Man-Month - 人月神话`**

### 没有银弹 (No Silver Bullet)

> 没有任何单一技术或管理方法能在十年内使软件生产力提升一个数量级。软件开发的根本困难在于概念性设计（思考做什么），而非实现（编码），后者可以通过工具改进，但前者难以被自动化。

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