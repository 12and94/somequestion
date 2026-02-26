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
