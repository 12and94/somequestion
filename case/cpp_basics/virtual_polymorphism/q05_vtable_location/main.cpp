#include <iostream>

struct Plain { int x; };
struct Poly { virtual ~Poly() = default; int x; };

int main() {
    // 多态类对象通常会多一个隐藏 vptr（实现相关）。
    std::cout << "sizeof(Plain)=" << sizeof(Plain) << "\n";
    std::cout << "sizeof(Poly)=" << sizeof(Poly) << "\n";
}
