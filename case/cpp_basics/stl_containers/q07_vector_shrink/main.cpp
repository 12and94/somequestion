// 题目：vector怎么释放空间？减容机制？
// 通俗理解：这题重点是步骤化表达：先做什么、再做什么、为什么这么做。

#include <iostream>
#include <vector>

int main() {
    std::vector<int> v(1000, 1);
    std::cout << v.capacity() << "\n";
    v.clear();
    std::vector<int>().swap(v); // 常见强制回收容量写法
    std::cout << v.capacity() << "\n";
}


