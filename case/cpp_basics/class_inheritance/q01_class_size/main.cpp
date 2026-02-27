// 题目：类的大小怎么计算？空类大小是多少？为什么是1字节？
// 通俗理解：这题重点是步骤化表达：先做什么、再做什么、为什么这么做。

#include <iostream>

struct Empty {};

struct A {
    int x;
    static int counter; // 不计入 sizeof(A)
};
int A::counter = 0;

int main() {
    std::cout << "sizeof(Empty)=" << sizeof(Empty) << "\n";
    std::cout << "sizeof(A)=" << sizeof(A) << "\n";
}


