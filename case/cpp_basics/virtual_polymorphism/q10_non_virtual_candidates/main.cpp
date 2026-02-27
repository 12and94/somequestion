// 题目：哪些函数不能是虚函数？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

#include <iostream>

struct X {
    X() = default;                 // 构造函数不能 virtual
    static void s() { std::cout << "static\n"; } // static 不能 virtual
    virtual void f() { std::cout << "member virtual\n"; }
};

void free_func() { std::cout << "free function\n"; } // 非成员不能 virtual

int main() {
    X x;
    x.f();
    X::s();
    free_func();
}


