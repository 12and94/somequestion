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
