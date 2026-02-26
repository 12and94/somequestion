#include <iostream>

void foo(int a, int b) {
    std::cout << a << " " << b << "\n";
}

int main() {
    int i = 0;
    // 不要依赖实参求值顺序：foo(i++, i++);
    foo(i + 1, i + 2);
}
