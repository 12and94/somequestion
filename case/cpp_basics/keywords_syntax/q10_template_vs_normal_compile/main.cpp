// 题目：模板函数和普通函数编译上的区别？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>

template <typename T>
T add(T a, T b) { return a + b; }

int add_int(int a, int b) { return a + b; }

int main() {
    std::cout << add(1.5, 2.5) << " " << add_int(1, 2) << "\n";
}


