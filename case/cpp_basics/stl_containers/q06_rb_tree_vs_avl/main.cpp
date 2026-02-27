// 题目：红黑树的原理？和AVL树的区别？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>
#include <map>

int main() {
    // map 通常基于红黑树，实现稳定 O(logN) 查改删。
    std::map<int, int> m;
    m[3] = 30;
    m[1] = 10;
    m[2] = 20;
    std::cout << m.begin()->first << "\n";
}


