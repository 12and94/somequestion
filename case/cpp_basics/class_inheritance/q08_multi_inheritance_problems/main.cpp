// 题目：多继承可能出现什么问题？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

#include <iostream>

struct B1 { void log() { std::cout << "B1\n"; } };
struct B2 { void log() { std::cout << "B2\n"; } };
struct D : B1, B2 {};

int main() {
    D d;
    // d.log(); // 二义性：来自 B1 和 B2
    d.B1::log();
    d.B2::log();
}


