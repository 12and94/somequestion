#include <iostream>

#if defined(_MSC_VER)
__forceinline int add1(int x) { return x + 1; }
#else
inline int add1(int x) { return x + 1; }
#endif

int main() {
    // force inline 是强烈建议，不是绝对保证。
    std::cout << add1(9) << "\n";
}
