#include <iostream>

inline int square_fn(int x) { return x * x; }
#define SQUARE_MACRO(x) ((x) * (x))

int main() {
    std::cout << square_fn(3) << " " << SQUARE_MACRO(3) << "\n";
}
