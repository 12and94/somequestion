// 题目：重载、重写、覆盖的区别？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

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


