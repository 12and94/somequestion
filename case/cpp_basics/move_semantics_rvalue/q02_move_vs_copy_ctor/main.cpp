// 题目：移动构造函数和拷贝构造函数的区别？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>
#include <vector>

int main() {
    std::vector<int> src(5, 7);
    std::vector<int> copy = src;           // 拷贝构造
    std::vector<int> moved = std::move(src); // 移动构造
    std::cout << copy.size() << " " << moved.size() << " " << src.size() << "\n";
}


