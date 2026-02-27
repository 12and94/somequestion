// 题目：析构函数为什么要是虚函数？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

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


