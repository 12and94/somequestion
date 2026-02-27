// 题目：结构体内存对齐规则？int, char, short的内存布局？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

#include <cstddef>
#include <iostream>

struct S {
    char c;
    int i;
    short s;
};

int main() {
    std::cout << "sizeof(S)=" << sizeof(S) << "\n";
    std::cout << "offset c/i/s = "
              << offsetof(S, c) << "/"
              << offsetof(S, i) << "/"
              << offsetof(S, s) << "\n";
}


