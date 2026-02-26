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
