#include <iostream>

struct Base {
    virtual void f(int) { std::cout << "Base::f(int)\n"; }
    void g(double) { std::cout << "Base::g(double)\n"; }
};

struct Derived : Base {
    void f(int) override { std::cout << "Derived::f(int)\n"; } // 重写
    void g(int) { std::cout << "Derived::g(int)\n"; }          // 隐藏
};

int main() {
    Derived d;
    d.f(1);
    d.g(2);
}
