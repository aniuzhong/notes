# **C/C++ 杂记** —— AI 时代，仍要保持记录

- [**C/C++ 杂记** —— AI 时代，仍要保持记录](#cc-杂记--ai-时代仍要保持记录)
  - [C 中的 `goto fail` 模式](#c-中的-goto-fail-模式)
  - [C++ 里传递对象建议方式](#c-里传递对象建议方式)
    - [函数返回值设计](#函数返回值设计)
  - [构造函数失败](#构造函数失败)
    - [1. 抛异常](#1-抛异常)
    - [2. 工厂函数](#2-工厂函数)
    - [3. 状态位 (std::ifstream)](#3-状态位-stdifstream)
    - [4. 两阶段初始化(旧式 C++ 风格)](#4-两阶段初始化旧式-c-风格)
  - [对象初始化](#对象初始化)
    - [1.区分 initializer\_list 构造函数](#1区分-initializer_list-构造函数)
    - [2.动态内存分配](#2动态内存分配)
    - [3.模板推导与 initializer\_list](#3模板推导与-initializer_list)
  - [异常安全(Exception safety)](#异常安全exception-safety)
  - [数组退化规则 + std::thread 的参数传递机制导致的问题](#数组退化规则--stdthread-的参数传递机制导致的问题)
  - [在模板参数推导语境下，重载函数名无法自动解析为唯一类型](#在模板参数推导语境下重载函数名无法自动解析为唯一类型)
  - [std::condition\_variable::wait 的原理](#stdcondition_variablewait-的原理)
  - [简单理解 ADL](#简单理解-adl)
    - [示例1](#示例1)
    - [示例2](#示例2)
  - [std::variant 的简单使用](#stdvariant-的简单使用)
  - [std::unique\_ptr 和 std::shared\_ptr 是否能够相互转换](#stdunique_ptr-和-stdshared_ptr-是否能够相互转换)
  - [The rule of three/five/zero](#the-rule-of-threefivezero)
    - [如果类中只包含 标准库资源 → 用 Rule of Zero。](#如果类中只包含-标准库资源--用-rule-of-zero)
    - [如果类中包含 裸系统资源（如 FILE\*） → 用 Rule of Five，并且通常禁止拷贝，只允许移动。](#如果类中包含-裸系统资源如-file--用-rule-of-five并且通常禁止拷贝只允许移动)
  - [Definitions and ODR (One Definition Rule)](#definitions-and-odr-one-definition-rule)
  - [合理使用 reserve() 可以避免 vector 在增长过程中频繁重新分配内存](#合理使用-reserve-可以避免-vector-在增长过程中频繁重新分配内存)
  - [Exit Thread of Loop (退出线程循环)](#exit-thread-of-loop-退出线程循环)
  - [Scope Guard](#scope-guard)
  - [了解 C++ 的内存模型](#了解-c-的内存模型)
  - [第三方库](#第三方库)
  - [📌跨平台访问 Unicode 文件名](#跨平台访问-unicode-文件名)
  - [📌标准并发库组件作为成员变量的类（属于资源管理类）](#标准并发库组件作为成员变量的类属于资源管理类)
  - [输出 Unicode 到终端](#输出-unicode-到终端)
    - [在各种文件编码下输出 `Hi,🌏`](#在各种文件编码下输出-hi)
  - [📌📌📌在 Windows 平台使用 spdlog 库的例子](#在-windows-平台使用-spdlog-库的例子)
  - [二进制兼容性（Binary Compatibility）](#二进制兼容性binary-compatibility)
    - [纯虚接口类 + C 工厂函数 -\> 不稳定](#纯虚接口类--c-工厂函数---不稳定)
    - [纯 C 接口 -\> 极其稳定](#纯-c-接口---极其稳定)
    - [COM 接口 -\> 极其稳定](#com-接口---极其稳定)
  - [C++ 并发工具的选择](#c-并发工具的选择)
    - [要返回值、要异常处理、短期任务 -\> std::async](#要返回值要异常处理短期任务---stdasync)
    - [要长期运行、要精细控制生命周期 -\> std::thread](#要长期运行要精细控制生命周期---stdthread)
    - [高性能处理大量小任务 -\> 线程池（可考虑 BS::thread\_pool）](#高性能处理大量小任务---线程池可考虑-bsthread_pool)
  - [推荐的头文件包含顺序](#推荐的头文件包含顺序)

---

## C 中的 `goto fail` 模式

无论程序在哪里出错，都通过 goto 跳转到函数末尾的一个统一标签，这是开源 C 项目（Linux 内核、ffmpeg）的指定写法

- 强制执行“后申请先释放”的栈式逻辑。
- 代码结构平整，不会产生极深的缩进

## C++ 里传递对象建议方式

| 用途 / 对象代价 | 复制代价低（如 int，或只能移动如 unique_ptr） | 移动代价低/中（如 vector、string，或模板中未知） | 移动代价高（如大结构体数组、std::array）|
| --- | --- | --- | --- |
| 出 (Out) | X f() | X f() | f(X&) |
| 入/出 (In/Out) | f(X&) | f(X&) | f(X&) |
| 入 (In) | f(X) | f(const X&) | f(const X&) |
| 入且保留一份 (In and keep a copy) | f(const X&) | f(const X&) + f(X&&) | f(const X&) |

- **出 (Out)** 函数“生产”一个值并返回给调用者。
- **入/出 (In/Out)** 函数既要读取参数，又要修改它，修改结果会反映到调用者的对象上。
- **入 (In)** 函数只需要读取参数，不会修改它。
- **入且保留一份 (In and keep a copy)** 函数需要读取参数，同时在函数内部保存一份副本（通常是成员变量），以便在函数返回后继续使用

### 函数返回值设计

老式写法:

```Cpp
bool f1(const std::string& in, std::string& out1, std::string& out2)
{
    if (in.size() == 0)
        return false;
    out1 = "hello";
    out2 = "world";
    return true;
}
```

C++17 新式写法

```Cpp
struct Out {
    std::string out1{""};
    std::string out2{""};
};

std::optional<Out> f2(const std::string& in) {
    Out o;
    if (in.size() == 0)
        return std::nullopt;
    o.out1 = "hello";
    o.out2 = "world";
    return { o };
}
```

## 构造函数失败

至今没有**唯一标准**，但有几种被广泛接受的模式：

### 1. 抛异常
### 2. 工厂函数

Protected/Private constructors with CreateInstance method.

### 3. 状态位 (std::ifstream)
### 4. 两阶段初始化(旧式 C++ 风格)

## 对象初始化

推荐统一用 `{}` 初始化。如避免 Most Vexing Parse：

```Cpp
std::string s();   // 注意！这是函数声明，不是对象
std::string s{};   // 正确，调用默认构造函数
std::string s2;    // 也正确，默认构造
```

仍然建议使用 `()` 的场景：

### 1.区分 initializer_list 构造函数

```Cpp
std::vector<int> v1(10, 1);  // 10 个元素，每个值为 1
std::vector<int> v2{10, 1};  // 两个元素：10 和 1
```

### 2.动态内存分配

```Cpp
auto p = new int(5);   // 分配并初始化为 5
auto q = new int{5};   // 也行，但传统写法更常见
```

### 3.模板推导与 initializer_list

``` Cpp
template<typename T>
void f(T t);

f(1);     // 推导为 int
f({1});   // 推导为 std::initializer_list<int>
```

## 异常安全(Exception safety)

[StackOverflow - Do you (really) write exception safe code?](https://stackoverflow.com/questions/1853243/do-you-really-write-exception-safe-code)

## 数组退化规则 + std::thread 的参数传递机制导致的问题

``` Cpp
//
// Print 0 to 9
//

void f(std::string str) { printf("%s ", str.c_str()); }

int main() {
    constexpr int N = 10;
    constexpr size_t L = 1024;
    std::vector<std::thread> thrds(N);
    for(int i = 0; i < N; ++i) {
        char buf[L];
        snprintf(buf, L, "%d", i);
        thrds[i] = std::thread(f, buf);
    }
    for(int i = 0; i < N; ++i)
        if (thrds[i].joinable())
            thrds[i].join();
    printf("\n");
    return 0;
}
```

一种改法是使用 std::array 代替数组

```Cpp
constexpr size_t L = 1024;
void f(const std::array<char, L>& arr) { printf("%s ", std::string(arr.data()).c_str()); }

int main() {
    constexpr int N = 10;
    std::vector<std::thread> thrds(N);
    for (int i = 0; i < N; ++i) {
        std::array<char, L> buf{};
        snprintf(buf.data(), buf.size(), "%d", i);
        thrds[i] = std::thread(f, buf);
    }
    for (int i = 0; i < N; ++i)
        if (thrds[i].joinable())
            thrds[i].join();
    printf("\n");
    return 0;
}
```

另一种是在创建线程时就构造出一个独立的 std::string

```Cpp
void f(std::string str) { printf("%s ", str.c_str()); }

int main() {
    constexpr int N = 10;
    constexpr size_t L = 1024;
    std::vector<std::thread> thrds(N);
    for (int i = 0; i < N; ++i) {
        char buf[L];
        snprintf(buf, L, "%d", i);
        thrds[i] = std::thread(f, std::string(buf));
    }
    for (int i = 0; i < N; ++i)
        if (thrds[i].joinable())
            thrds[i].join();
    printf("\n");
    return 0;
}
```

## 在模板参数推导语境下，重载函数名无法自动解析为唯一类型

[C++ - compilation fails on calling overloaded function in std::thread](https://stackoverflow.com/questions/44049407/c-compilation-fails-on-calling-overloaded-function-in-stdthread)

```Cpp
void foo(int) {};
void foo(double) {};
std::function<void(int)> f = static_cast<void(*)(int)>(&foo);
std::function<void(int)> g = foo; // Error
```

## std::condition_variable::wait 的原理

```Text
Atomically calls lock.unlock() and blocks on *this. The thread will be unblocked when notify_all() or notify_one() is executed. It may also be unblocked spuriously.
 When unblocked, calls lock.lock() (possibly blocking on the lock), then returns.
```

```Cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

std::mutex mtx;
std::condition_variable cv;
bool ready = false;  // 条件变量依赖的共享状态

void worker(int id) {
    std::unique_lock<std::mutex> lock(mtx);

    // ====== 等待过程的规律 ======
    // 1. wait 会原子地释放锁 (lock.unlock())，然后阻塞在条件变量上。
    //    这样避免了释放锁和进入等待之间的竞争条件。
    // 2. 线程会在以下情况被唤醒：
    //    - 其他线程调用 notify_one() 或 notify_all()
    //    - 或者虚假唤醒 (spurious wakeup)，即没有通知也可能返回
    // 3. 被唤醒后，wait 会重新尝试获取锁 (lock.lock())。
    //    如果锁被其他线程占用，它可能会再次阻塞，直到拿到锁。
    // 4. 拿到锁后，wait 才会返回，继续执行后续代码。
    //
    // 因为可能出现虚假唤醒，所以必须在循环里检查条件，
    // 或者使用带谓词的 wait(lock, predicate)，确保条件满足才继续。

    cv.wait(lock, [] { return ready; });  // 带谓词的写法，避免虚假唤醒

    // 唤醒后，线程已经重新持有锁，并且 ready == true
    std::cout << "Worker " << id << " is running\n";
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = true;  // 改变条件
    }
    cv.notify_all();  // 唤醒所有等待的线程

    t1.join();
    t2.join();
    return 0;
}
```

## 简单理解 ADL

[Argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl)

### 示例1

```Cpp
#include <iostream>
#include <string>

namespace Domain {
    struct User {
        std::string name;
        int id;
    };

    // 在 Domain 命名空间里定义比较操作
    bool operator==(const User& a, const User& b) {
        return a.id == b.id;
    }
}

int main() {
    Domain::User u1{"Alice", 1};
    Domain::User u2{"Bob", 1};

    // 这里直接写 u1 == u2
    // 编译器会通过 ADL 在 Domain 命名空间里找到 operator==
    if (u1 == u2) {
        std::cout << "Same user\n";
    } else {
        std::cout << "Different user\n";
    }
}
```

### 示例2

```Cpp
#include <iostream>
#include <string>

namespace Domain {
    struct User {
        std::string name;
        int id;
    };

    // 在 Domain 命名空间里定义输出操作
    std::ostream& operator<<(std::ostream& os, const User& u) {
        return os << "User(" << u.name << ", " << u.id << ")";
    }
}

int main() {
    Domain::User u{"Alice", 42};

    // 这里直接写 std::cout << u
    // 编译器通过 ADL 在 Domain 命名空间里找到 operator<<
    std::cout << u << std::endl;
}
```

## std::variant 的简单使用

Typical use of [std::visit](https://en.cppreference.com/w/cpp/utility/variant/visit2)

```C++
using var_t = std::variant<int, long, double, std::string>;
 
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
 
int main()
{
    std::vector<var_t> vec = {10, 15l, 1.5, "hello"};
    for (auto& v: vec)
    {
        std::visit(
            // Aggregate classes with base classes (since C++17)
            // By inheriting from these lambdas,
            // the overloaded struct can use their operator() functions.
            overloaded{ [](auto arg) { std::cout << arg << ' '; },
                        [](double arg) { std::cout << std::fixed << arg << ' '; },
                        [](const std::string& arg) { std::cout << std::quoted(arg) << ' '; } },
            v);
    }
}
```
## std::unique_ptr 和 std::shared_ptr 是否能够相互转换

[StackOverflow - Does C++11 unique_ptr and shared_ptr able to convert to each other's type?](https://stackoverflow.com/questions/37884728/does-c11-unique-ptr-and-shared-ptr-able-to-convert-to-each-others-type)

- `unique_ptr` → `shared_ptr`
  - 可以通过构造函数将一个 unique_ptr 转换为 shared_ptr。
  - 转换后，shared_ptr 接管对象的所有权，而原来的 unique_ptr 会被释放（变为空）。
  - 这是一个 单向且安全的转换，因为 shared_ptr 可以管理多个引用，而 unique_ptr 保证唯一所有权。
- `shared_ptr` → `unique_ptr`
  - 不允许直接转换。
  - 原因是 shared_ptr 可能有多个副本，无法保证“唯一所有权”，因此不能安全地转成 unique_ptr。
  - 如果确实需要，可以通过 shared_ptr::unique() 检查是否只有一个引用，然后用 std::move 来获取底层指针，但这不是标准提供的直接转换。


```Cpp
#include <memory>
#include <iostream>

struct Base {
    virtual void foo() = 0;
    virtual ~Base() = default;
};

struct DerivedA : Base {
    void foo() override { std::cout << "DerivedA\n"; }
};

struct DerivedB : Base {
    void foo() override { std::cout << "DerivedB\n"; }
};

// 工厂函数
std::unique_ptr<Base> makeObject(int type) {
    if (type == 1) return std::make_unique<DerivedA>();
    else return std::make_unique<DerivedB>();
}

int main() {
    auto obj = makeObject(1);
    obj->foo();  // 输出 DerivedA

    // 如果需要共享所有权
    std::shared_ptr<Base> sharedObj = std::move(obj);
    sharedObj->foo(); // 仍然有效
}

```

## The rule of three/five/zero

[CppReference](https://en.cppreference.com/w/cpp/language/rule_of_three)

- 标准库资源 → Rule of Zero
- 裸系统资源 → Rule of Five，禁止拷贝，只允许移动

实践中更推荐的模式是：先写一个小的 RAII 封装类来管理裸资源 → 然后业务类依然可以遵循 Rule of Zero。

### 如果类中只包含 标准库资源 → 用 Rule of Zero。

```Cpp
#include <string>
#include <vector>

// ✅ 不需要写析构函数、拷贝构造、赋值运算符

class RuleOfZero {
private:
    std::string name;
    std::vector<int> data;

public:
    RuleOfZero(std::string n, std::vector<int> d)
        : name(std::move(n)), data(std::move(d)) {}

    void print() const {
        for (auto v : data) {
            std::cout << v << " ";
        }
        std::cout << "\n";
    }
};
```

### 如果类中包含 裸系统资源（如 FILE*） → 用 Rule of Five，并且通常禁止拷贝，只允许移动。

```Cpp
#include <cstdio>
#include <stdexcept>
#include <string>

class FileWrapper {
private:
    FILE* file;

public:
    // 构造函数：打开文件
    FileWrapper(const std::string& filename, const char* mode) {
        file = std::fopen(filename.c_str(), mode);
        if (!file) {
            throw std::runtime_error("Failed to open file");
        }
    }

    // 析构函数：关闭文件
    ~FileWrapper() {
        if (file) {
            std::fclose(file);
        }
    }

    // 禁止拷贝（因为 FILE* 不能安全共享）
    FileWrapper(const FileWrapper&) = delete;
    FileWrapper& operator=(const FileWrapper&) = delete;

    // 允许移动（转移资源所有权）
    FileWrapper(FileWrapper&& other) noexcept : file(other.file) {
        other.file = nullptr;
    }

    FileWrapper& operator=(FileWrapper&& other) noexcept {
        if (this != &other) {
            if (file) {
                std::fclose(file);
            }
            file = other.file;
            other.file = nullptr;
        }
        return *this;
    }

    // 提供操作接口
    void write(const std::string& text) {
        if (file) {
            std::fputs(text.c_str(), file);
        }
    }
};
```

## Definitions and ODR (One Definition Rule)

[CppReference](https://en.cppreference.com/w/cpp/language/definition)

> 在头文件中实现的函数需要加 inline 才可以编译过

## 合理使用 reserve() 可以避免 vector 在增长过程中频繁重新分配内存

```Cpp
#include <vector>
#include <string>

int main() {
    std::vector<std::string> v;
    v.reserve(1000); // 预留空间

    for (int i = 0; i < 1000; ++i) {
        v.push_back("hello");
    }
}
```

## Exit Thread of Loop (退出线程循环)

```Cpp
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

std::atomic<bool> stopFlag(false);

void worker() {
    while (!stopFlag.load()) {
        std::cout << "Working..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "Thread cleanup and exit." << std::endl;
}

int main() {
    std::thread t(worker);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    stopFlag.store(true);   // 通知退出
    t.join();               // 等待线程结束
    return 0;
}
```

## Scope Guard

[Generic: Change the Way You Write Exception-Safe Code — Forever](https://erdani.org/publications/cuj-12-2000.php.html)

- [Folly.ScopeExit](https://github.com/facebook/folly/blob/main/folly/ScopeGuard.h)
- [LFTS v3](https://en.cppreference.com/w/cpp/experimental/scope_exit)
- Boost.ScopeExit

[讨论 - 1](https://codereview.stackexchange.com/questions/245846/c20-scopeguard)

## 了解 C++ 的内存模型

[理解 C++ 的 Memory Order](https://senlinzhan.github.io/2017/12/04/cpp-memory-order)
[如何理解 C++11 的六种 memory order？](https://www.zhihu.com/question/24301047)

## 第三方库

[tanakh/cmdline: A Command Line Parser](https://github.com/tanakh/cmdline)

[jarro2783/cxxopts: Lightweight C++ command line option parser](https://github.com/jarro2783/cxxopts)

[uni-algo: Unicode Algorithms Implementation for C/C++](https://github.com/uni-algo/uni-algo)

[nemtrif/utfcpp: UTF-8 with C++ in a Portable Way](https://github.com/nemtrif/utfcpp)

[gabime/spdlog: ast C++ logging library](https://github.com/gabime/spdlog)

[renatoGarcia/icecream-cpp: 🍦Never use cout/printf to debug again](https://github.com/renatoGarcia/icecream-cpp.git)

[smasherprog/screen_capture_lite: cross platform screen/window capturing library](https://github.com/smasherprog/screen_capture_lite)

``` Sh
sudo apt install libxfixes-dev
```

[serge-rgb/TinyJPEG: Single header lib for JPEG encoding](https://github.com/serge-rgb/TinyJPEG)

[libsigc++: A typesafe callback system for standard C++](https://github.com/libsigcplusplus/libsigcplusplus)

``` Sh
sudo apt install libsigc++-3.0-dev
```

## 📌跨平台访问 Unicode 文件名

C/C++ 在不同系统上处理 **Unicode 文件名** 一直是个“各自为政”的局面：**没有统一标准、完全依赖平台 API**。核心思路是：

> Windows 用 UTF‑16 宽字符 API；类 Unix 系统用 UTF‑8 字节序列。

这部分代码可以参考 libobs 的实现

```
                +----------------------+
                |   os_fopen (UTF‑8)   |
                +----------------------+
                           |
          +----------------+----------------+
          |                                 |
      Windows                           POSIX
          |                                 |
          v                                 v
+----------------------+          +----------------------+
| utf8 → wcs (convert) |          |   fopen(utf8_path)   |
+----------------------+          +----------------------+
          |
          v
+----------------------+
|   os_wfopen (WCS)    |
+----------------------+
          |
   +------+------+
   |             |
Windows       POSIX
   |             |
   v             v
 _wfopen     wcs → utf8 → fopen
```

C++17 后可以使用 std::filesystem::path 来表示 Unicode 文件名。

C++20 后支持 UTF-8 字面量。

> 将路径名仅使用 ASCII 码会节省很多不必要的麻烦

## 📌标准并发库组件作为成员变量的类（属于资源管理类）

自定义类里包含了 `std::mutex`、`std::condition_variable` 中的任意一个，它的默认拷贝构造、拷贝赋值、移动构造、移动赋值都会被编译器自动标记为 **deleted**。

这部分代码可以参考 OBS 中实现 WHIP 推流输出模块的类 WHIPOutput

```C++
class WHIPOutput {
public:
    WHIPOutput(obs_data_t* settings, obs_output_t* output);
    ~WHIPOutput();

    bool Start();
    void Stop(bool signal = true);
    void Data(struct encoder_packet* packet);

    inline size_t GetTotalBytes() { return total_bytes_sent; }

    inline int GetConnectTime() { return connect_time_ms; }

private:
    void ConfigureAudioTrack(std::string media_stream_id, std::string cname);
    void ConfigureVideoTrack(std::string media_stream_id, std::string cname);
    bool Init();
    bool Setup();
    bool Connect();
    void StartThread();
    void SendDelete();
    void StopThread(bool signal);
    void ParseLinkHeader(std::string linkHeader, std::vector<rtc::IceServer>& iceServers);
    void Send(void* data, uintptr_t size, uint64_t duration, std::shared_ptr<rtc::Track> track,
              std::shared_ptr<rtc::RtcpSrReporter> rtcp_sr_reporter);

    obs_output_t* output;

    std::string endpoint_url;
    std::string bearer_token;
    std::string resource_url;

    std::atomic<bool> running;

    std::mutex start_stop_mutex;
    std::thread start_stop_thread;

    uint32_t base_ssrc;
    std::shared_ptr<rtc::PeerConnection> peer_connection;
    std::shared_ptr<rtc::Track> audio_track;
    std::shared_ptr<rtc::Track> video_track;
    std::shared_ptr<rtc::RtcpSrReporter> audio_sr_reporter;
    std::shared_ptr<rtc::RtcpSrReporter> video_sr_reporter;

    std::map<obs_encoder_t*, std::shared_ptr<videoLayerState>> videoLayerStates;

    std::atomic<size_t> total_bytes_sent;
    std::atomic<int> connect_time_ms;
    int64_t start_time_ns;
    int64_t last_audio_timestamp;
};
```

## 输出 Unicode 到终端

> 难点：在不同平台上正确选择编码并让控制台解码成功

1. 终端/控制台只处理窄字符和宽字符；
2. 在 Windows 平台上，wchar_t 通常是 16 位（2字节），用于表示 UTF-16 编码的字符；
3. 在大多数 Linux 和 Unix 系统上，wchar_t 通常是 32 位（4字节），用于表示 UTF-32 编码的字符;
4. std::cout 和 printf 本身并不包含任何编码信息，仅会将原始字节（Raw Bytes）发送到终端；
5. 源代码在编译时的字符编码也会影响输出结果
6. 字符串编码检测不是纯客观的判定，而是合法性验证 + 启发式猜测 + 外部信息结合

### 在各种文件编码下输出 `Hi,🌏`

```C++
#ifdef _WIN32
#include <windows.h>
#else
#include <boost/locale.hpp>
#endif

#include <iostream>
#include <string>

void os_write_console(const char* data, size_t size) {
#ifdef _WIN32
    UINT output_cp_old = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
    DWORD written;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), data, (DWORD)size, &written, nullptr);
    SetConsoleOutputCP(output_cp_old);
#else
    std::cout.write(data, size);
#endif
}

void os_write_console(const std::string& str) {
    os_write_console(str.data(), str.size());
}

void os_write_console(const std::u8string& u8str) {
    os_write_console(reinterpret_cast<const char*>(u8str.c_str()), u8str.size());
}

void os_write_console(const std::wstring& wstr) {
#ifdef _WIN32
    DWORD written;
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), wstr.c_str(), (DWORD)wstr.size(), &written, nullptr);
#else
    // Linux/macOS: convert wstring (UTF-32) to UTF-8
    std::string utf8 = boost::locale::conv::utf_to_utf<char>(wstr);
    std::cout << utf8;
#endif
}

void os_write_console(const std::u16string& u16str) {
#ifdef _WIN32
    DWORD written;
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                  reinterpret_cast<const wchar_t*>(u16str.c_str()),
                  (DWORD)u16str.size(), &written, nullptr);
#else
    std::string utf8 = boost::locale::conv::utf_to_utf<char>(u16str);
    std::cout << utf8;
#endif
}

int main()
{
    // 为了不受文件编码影响，🌏 需要写为 Unicode 转义序列或逐字节写出 UTF-8 编码

    // 宽字符串
    os_write_console(L"Hi, \U0001F30F\n");

    // UTF-8 字符串
    os_write_console(u8"Hi, \U0001F30F\n");

    // UTF-16 字符串 (std::u16string)
    os_write_console(u"Hi, \U0001F30F\n");

    // 普通窄字符串 (约定 UTF-8 编码)
    os_write_console("Hi, \xF0\x9F\x8C\x8F\n");
}
```

## 📌📌📌在 Windows 平台使用 spdlog 库的例子

```C++
#include <iostream>
#include <chrono>
#include <filesystem>

#pragma execution_character_set("utf-8")

#define SPDLOG_WCHAR_FILENAMES
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

static_assert(L"测试"[0] == L'\u6D4B', "Source encoding mismatch for U+6D4B");
static_assert(U"测试"[0] == U'\u6D4B', "Source encoding mismatch for U+6D4B");
static_assert(u"测试"[0] == u'\u6D4B', "Source encoding mismatch for U+6D4B");

std::string normalize_exception_message(const char* msg)
{
#ifdef _WIN32
    UINT cp = GetACP();
    if (cp == 936)
    {
        int len = MultiByteToWideChar(cp, 0, msg, -1, nullptr, 0);
        std::wstring wstr(len, L'\0');
        MultiByteToWideChar(cp, 0, msg, -1, &wstr[0], len);
        len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string utf8(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], len, nullptr, nullptr);
        return utf8;
    }
#endif
    return std::string(msg);
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    auto temp = std::filesystem::temp_directory_path();
    auto log_dir = std::filesystem::path(temp) / L"临时" / L"日志";

    std::shared_ptr<spdlog::logger> logger;

    try
    {
        std::filesystem::create_directories(log_dir);

        auto log_file = log_dir / L"测试日志.log";
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file.wstring(), true);
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        std::vector<spdlog::sink_ptr> sinks{ file_sink, console_sink };
        logger = std::make_shared<spdlog::logger>("MultiSinkLogger", sinks.begin(), sinks.end());

        spdlog::register_logger(logger);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        logger = spdlog::stdout_color_mt("ConsoleLogger");
        logger->error("{}", normalize_exception_message(e.what()));
    }
    catch (const spdlog::spdlog_ex& e)
    {
        logger = spdlog::stdout_color_mt("ConsoleLogger");
        logger->error("{}", normalize_exception_message(e.what()));
    }

    if (!logger)
        logger = spdlog::stdout_color_mt("ConsoleLogger");

    logger->warn("这是一条警告日志");
    logger->info("这是一条通知日志");

    return 0;
}
```

以上代码做了**很多**细节处理：

- execution_character_set("utf-8") 且增加额外的 `/utf-8` 选项指定编译时的字符集（可能不奏效）。
- 定义 `SPDLOG_WCHAR_FILENAMES` 宏，使 `spdlog::filename_t` 定义为 std::wstring。迎合 Windows 平台字符串编码习惯。
- 静态断言 `L"测试"` 在编译时是否被编码为 `{L'\u6D4B', L'\u8BD5', 0}`，来强制约束文件编码为 UTF-8。
- SetConsoleOutputCP(CP_UTF8) 将控制台输出的代码页改为 UTF-8，适应 spdlog 的 UTF-8 输出。
- 考虑到异常信息在中文 Windows 下为 GBK 编码，因此用 normalize_exception_message 转换为 UTF-8。

事实上，即便强制用字符串前缀（u8、u、U）指定编码格式，也**不能**保证字符串编码和预期一致。 所以，`spdlog::logger::info` 输出非 ASCII 字面量时仍有可能由于文件编码不是 UTF-8 而导致乱码。

虽然考虑到了很多情况，但是上述代码仍不能确保万无一失。

> ※ 从经验上来讲，通过以下做法可以减少很多不必要的麻烦：

- 文件统一使用**不带签名的 UTF-8** 进行编码（即使在 Windows 系统或某些应用中，带 BOM 的 UTF-8 文件更容易被正确识别）。
- C/C++ 源文件的**注释统一使用 ASCII 码**。
- **写日志尽量全部使用英文**。
- **字符串常量尽量使用英文**，除非有需求。
- **Win32 编程中统一使用宽字符版 API**，但仍尽量使用英文。

另外，C++ 标准库里凡是涉及**人类可读信息**（如异常错误消息），几乎都没有规定语言和编码，属于**实现定义**。最好不去依赖这些信息，如果必须依赖则需要小心处理。

## 二进制兼容性（Binary Compatibility）

> C++ 标准不涉及二进制兼容性。

### 纯虚接口类 + C 工厂函数 -> 不稳定

[Binary-compatible C++ Interfaces](https://chadaustin.me/cppinterface.html)

```c++
#ifndef CALCULATOR_H
#define CALCULATOR_H

class ICalculator {
public:
    ICalculator() = default;
    ICalculator(const ICalculator&) = delete;
    ICalculator& operator=(const ICalculator&) = delete;
    ICalculator(ICalculator&&) = delete;
    ICalculator& operator=(ICalculator&&) = delete;
    virtual int Add(int a, int b) = 0;
    virtual int Sub(int a, int b) = 0;
protected:
    virtual ~ICalculator() = default;
};

extern "C" {
__declspec(dllexport) ICalculator* CreateCalculator();
__declspec(dllexport) void         DestroyCalculator(ICalculator* p);
}

#endif // CALCULATOR_H
```

```c++
#include "Calculator.h"

class CalculatorImpl : public ICalculator
{
public:
    virtual ~CalculatorImpl() {}
    virtual int Add(int a, int b) { return a + b; }
    virtual int Sub(int a, int b) { return a - b; }
};

extern "C" ICalculator* CreateCalculator()
{
    return new CalculatorImpl();
}

extern "C" void DestroyCalculator(ICalculator* p)
{
    delete p;
}
```

### 纯 C 接口 -> 极其稳定

### COM 接口 -> 极其稳定

## C++ 并发工具的选择

### 要返回值、要异常处理、短期任务 -> std::async

async 没有性能优势，它存在的意义是：当你需要从异步任务中安全地获取返回值或捕获异常时，async 是无可替代的。

```C++
double compute(int id) {
    if (id < 0) throw std::invalid_argument("id 必须为非负数");
    return id * 1.5 + 42;
}

int main() {
    // 显式指定 std::launch::async
    // 否则默认策略 (async | deferred) 可能延迟执行
    auto f1 = std::async(std::launch::async, compute, 10);
    auto f2 = std::async(std::launch::async, compute, -5);

    try {
        std::cout << "结果 1: " << f1.get() << "\n"; // 正常输出
        std::cout << "Result 2: " << f2.get() << "\n"; // 抛出异常！
    } catch (const std::exception& e) {
        std::cout << "Caught: " << e.what() << "\n";
    }
}
```

这段代码简洁、安全、无需手动同步。

异常会在子线程中抛出，在主线程中被捕获，这是 std::thread 无法做到。

不过这里藏着一个坑，就是你调用 std::async 之后，如果忽略返回的 std::future，任务会同步执行。

```C++
void trap() {
    std::async(std::launch::async, []{
        std::cout << "开始\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "结束\n";
    });
    std::cout << "主函数完成\n";
}
```

你以为会输出：

```
Main done
Start
End
```

实际输出却是：

```
Start
（等待两秒）
End
Main done
```

因为临时的 std::future 在语句结束时立即析构，而 future 的析构函数会阻塞等待任务完成。于是整个调用变成同步串行。

这是 C++ 标准有意设计的，目的是防止 fire-and-forget 导致资源泄漏。

正确做法只有两个：

1、保存 future，稍后 get 或 wait

2、改用 std::thread 并通过 RAII 对象管理其生命周期，而不是裸用 detach。

### 要长期运行、要精细控制生命周期 -> std::thread

```C++
class BackgroundWorker {
    std::atomic<bool> running{true};
    std::thread worker;

    void loop() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            // do_work();
        }
    }

public:
    BackgroundWorker() : worker(&BackgroundWorker::loop, this) {}

    void stop() {
        running = false;
        if (worker.joinable()) worker.join();
    }

    ~BackgroundWorker() {
        stop();
    }
};
```

### 高性能处理大量小任务 -> 线程池（可考虑 BS::thread_pool）

下面是一个值得学习的实现，不过在生产环境需增加队列限容、异常隔离等：

```C++
// 示例，生产环境应增加：
// 1. 任务队列容量限制防 OOM
// 2. submit 失败时的 fallback 策略
// 3. 单个任务异常不应影响整个池
class TaskPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv_task;
    bool stop = false;

public:
    explicit TaskPool(size_t threads = std::thread::hardware_concurrency())
        : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        cv_task.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<class F, class... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<return_type> res = task->get_future();
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (stop) throw std::runtime_error("在已停止的任务池中提交任务");
            tasks.emplace([task]() { (*task)(); });
        }
        cv_task.notify_one();
        return res;
    }

    ~TaskPool() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            stop = true;
        }
        cv_task.notify_all();
        for (auto& t : workers) t.join();
    }
};
```

## 推荐的头文件包含顺序

业界惯例（Google、LLVM 等风格指南一致）：

1. 对应的本文件头文件（如 foo.cpp 先 include "foo.hpp"）
2. C 标准库      <cmath> <cstdio> <cstdint>
3. C++ 标准库    <vector> <span> <memory>
4. 第三方库      <glm/glm.hpp> <glad/glad.h> <GLFW/glfw3.h>
5. 本项目头文件  "item.hpp" "renderer.hpp"