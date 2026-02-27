// 题目：map和unordered_map的区别？底层实现？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>
#include <map>
#include <unordered_map>

int main() {
    std::map<int, int> ordered{{2, 20}, {1, 10}};
    std::unordered_map<int, int> hashed{{2, 20}, {1, 10}};
    std::cout << ordered.begin()->first << " " << hashed.size() << "\n";
}


