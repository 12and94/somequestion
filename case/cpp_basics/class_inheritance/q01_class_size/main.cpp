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
