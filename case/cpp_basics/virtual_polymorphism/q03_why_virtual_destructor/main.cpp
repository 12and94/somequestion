#include <iostream>

struct Base {
    virtual ~Base() { std::cout << "~Base\n"; }
};

struct Derived : Base {
    ~Derived() override { std::cout << "~Derived\n"; }
};

int main() {
    // 通过基类指针删除派生对象，必须依赖虚析构。
    Base* p = new Derived();
    delete p;
}
