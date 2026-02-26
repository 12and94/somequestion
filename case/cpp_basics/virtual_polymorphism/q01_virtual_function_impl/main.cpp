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
