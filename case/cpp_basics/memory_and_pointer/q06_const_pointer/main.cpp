// 题目：const char* 和 char* const的区别？底层const和顶层const？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>

int main() {
    int a = 1;
    int b = 2;

    const int* p1 = &a; // 底层 const：不能通过 p1 改值
    int* const p2 = &a; // 顶层 const：p2 不能改指向

    p1 = &b;
    *p2 = 10;
    std::cout << *p1 << " " << *p2 << "\n";
}


