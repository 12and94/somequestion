// 题目：const的用法？const函数修饰的是什么？
// 通俗理解：先下定义，再给一个实际场景，面试会更有说服力。

#include <iostream>

int main() {
    int x = 10;
    const int* p1 = &x; // 不能通过 p1 改 x
    int* const p2 = &x; // p2 不能改指向
    *p2 = 20;
    std::cout << *p1 << " " << *p2 << "\n";
}


