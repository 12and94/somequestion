#include <iostream>

template <typename T>
T add(T a, T b) { return a + b; }

int add_int(int a, int b) { return a + b; }

int main() {
    std::cout << add(1.5, 2.5) << " " << add_int(1, 2) << "\n";
}
