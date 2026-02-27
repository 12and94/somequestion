// 题目：nullptr上调用成员函数会怎样？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

#include <iostream>

struct A {
    void run() { std::cout << "run\n"; }
};

int main() {
    // A* p = nullptr; p->run(); // 未定义行为，示例中不要执行。
    A a;
    a.run();
}


