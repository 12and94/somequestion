// 题目：final和override关键字的作用？
// 通俗理解：回答时先说它解决的核心痛点，再说副作用或限制。

#include <iostream>

struct Base {
    virtual ~Base() = default;
    virtual void run() { std::cout << "Base\n"; }
};

struct Derived final : Base {
    void run() override { std::cout << "Derived\n"; }
};

int main() {
    Derived d;
    d.run();
}


