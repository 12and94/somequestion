#include <iostream>

struct A { int x = 1; };
struct B : virtual A {};
struct C : virtual A {};
struct D : B, C {};

int main() {
    D d;
    d.x = 42; // 虚继承后只有一份 A::x
    std::cout << d.B::x << " " << d.C::x << "\n";
}
