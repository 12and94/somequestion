// 题目：vector的reserve和resize的区别？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>
#include <vector>

int main() {
    std::vector<int> v;
    v.reserve(100);
    std::cout << "size=" << v.size() << ", cap=" << v.capacity() << "\n";
    v.resize(3);
    std::cout << "size=" << v.size() << ", cap=" << v.capacity() << "\n";
}


