// 题目：创建10个实例有几个虚函数表？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

#include <iostream>

struct Base {
    virtual ~Base() = default;
    virtual void f() {}
};

int main() {
    Base a, b, c;
    // 同类型对象通常共享一张 vtable，每个对象各有自己的 vptr。
    std::cout << &a << "\n" << &b << "\n" << &c << "\n";
}


