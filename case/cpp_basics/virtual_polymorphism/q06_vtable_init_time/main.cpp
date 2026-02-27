// 题目：虚函数表什么时候初始化？
// 通俗理解：关键是时机：在生命周期哪一阶段发生、由谁触发。

#include <iostream>

struct Base {
    Base() { who(); }
    virtual void who() { std::cout << "Base stage\n"; }
    virtual ~Base() = default;
};

struct Derived : Base {
    Derived() { who(); }
    void who() override { std::cout << "Derived stage\n"; }
};

int main() {
    // 构造链中 vptr 会按阶段变化。
    Derived d;
}


