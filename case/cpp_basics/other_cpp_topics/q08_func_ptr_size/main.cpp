#include <iostream>

struct A { void f() {} };

int free_fn() { return 0; }

int main() {
    using FP = int (*)();
    using MFP = void (A::*)();
    std::cout << sizeof(FP) << " " << sizeof(MFP) << "\n";
    (void)free_fn;
}
