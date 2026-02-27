// 题目：虚函数表存放在哪里？是每个类一份还是每个对象一份？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

#include <iostream>

struct Plain { int x; };
struct Poly { virtual ~Poly() = default; int x; };

int main() {
    // 多态类对象通常会多一个隐藏 vptr（实现相关）。
    std::cout << "sizeof(Plain)=" << sizeof(Plain) << "\n";
    std::cout << "sizeof(Poly)=" << sizeof(Poly) << "\n";
}


