#include <cstddef>
#include <iostream>

struct S {
    char c;
    int i;
};

int main() {
    std::cout << sizeof(S) << " " << offsetof(S, i) << "\n";
}
