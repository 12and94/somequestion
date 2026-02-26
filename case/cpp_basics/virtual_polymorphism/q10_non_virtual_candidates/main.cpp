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
