#include <iostream>

int main() {
    int x = 10;
    const int* p1 = &x; // 不能通过 p1 改 x
    int* const p2 = &x; // p2 不能改指向
    *p2 = 20;
    std::cout << *p1 << " " << *p2 << "\n";
}
