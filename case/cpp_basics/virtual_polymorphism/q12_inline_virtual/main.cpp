// 题目：inline可以修饰虚函数吗？
// 通俗理解：这题通常先给结论，再解释语言规则或机制原因。

#include <iostream>

struct Base {
    virtual ~Base() = default;
    inline virtual void ping() { std::cout << "Base::ping\n"; }
};

struct Derived : Base {
    inline void ping() override { std::cout << "Derived::ping\n"; }
};

int main() {
    // inline + virtual 合法；是否真正内联由编译器决定。
    Base* p = new Derived();
    p->ping();
    delete p;
}


