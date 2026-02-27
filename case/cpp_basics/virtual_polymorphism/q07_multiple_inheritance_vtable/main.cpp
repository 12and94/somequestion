// 题目：多重继承的虚函数表怎么存？有几张表？
// 通俗理解：这题重点是步骤化表达：先做什么、再做什么、为什么这么做。

#include <iostream>

struct B1 { virtual ~B1() = default; virtual void f() { std::cout << "B1::f\n"; } };
struct B2 { virtual ~B2() = default; virtual void g() { std::cout << "B2::g\n"; } };

struct D : B1, B2 {
    void f() override { std::cout << "D::f\n"; }
    void g() override { std::cout << "D::g\n"; }
};

int main() {
    D d;
    B1* p1 = &d;
    B2* p2 = &d;
    p1->f();
    p2->g();
}


