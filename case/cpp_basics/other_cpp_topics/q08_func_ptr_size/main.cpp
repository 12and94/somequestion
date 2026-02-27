// 题目：函数指针和成员函数指针的大小？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

#include <iostream>

struct A { void f() {} };

int free_fn() { return 0; }

int main() {
    using FP = int (*)();
    using MFP = void (A::*)();
    std::cout << sizeof(FP) << " " << sizeof(MFP) << "\n";
    (void)free_fn;
}


