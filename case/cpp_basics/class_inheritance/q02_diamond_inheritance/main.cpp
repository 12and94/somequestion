// 题目：菱形继承是什么？如何解决？
// 通俗理解：这题重点是步骤化表达：先做什么、再做什么、为什么这么做。

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


