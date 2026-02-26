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
