// 题目：force inline是否一定内联？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

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


