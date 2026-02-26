#include <iostream>
#include <utility>

void sink(int&) { std::cout << "lvalue\n"; }
void sink(int&&) { std::cout << "rvalue\n"; }

template <typename T>
void forward_to_sink(T&& x) {
    sink(std::forward<T>(x)); // 保留值类别
}

int main() {
    int a = 1;
    forward_to_sink(a);
    forward_to_sink(2);
}
