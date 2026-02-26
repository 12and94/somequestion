#include <iostream>

struct A {
    void run() { std::cout << "run\n"; }
};

int main() {
    // A* p = nullptr; p->run(); // 未定义行为，示例中不要执行。
    A a;
    a.run();
}
