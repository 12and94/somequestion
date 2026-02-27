// 题目：inline函数的优缺点？和宏的区别？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>

inline int square_fn(int x) { return x * x; }
#define SQUARE_MACRO(x) ((x) * (x))

int main() {
    std::cout << square_fn(3) << " " << SQUARE_MACRO(3) << "\n";
}


