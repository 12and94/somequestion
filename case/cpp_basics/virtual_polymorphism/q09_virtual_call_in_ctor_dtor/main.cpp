#include <iostream>

struct Base {
    Base() { call(); }
    virtual ~Base() { call(); }
    virtual void call() { std::cout << "Base::call\n"; }
};

struct Derived : Base {
    void call() override { std::cout << "Derived::call\n"; }
};

int main() {
    // 构造/析构阶段调用虚函数，只会走当前阶段版本。
    Derived d;
}
