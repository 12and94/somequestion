// 题目：虚函数的实现原理？虚函数表是什么？
// 通俗理解：按“数据从哪里来、经过什么步骤、最后得到什么结果”来理解最稳。

#include <iostream>

struct Base {
    virtual ~Base() = default;
    virtual void draw() const { std::cout << "Base\n"; }
};

struct Circle : Base {
    void draw() const override { std::cout << "Circle\n"; }
};

int main() {
    // 基类指针指向派生对象，运行时分发到 Circle::draw。
    Base* p = new Circle();
    p->draw();
    delete p;
}


