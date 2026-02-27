// 题目：vector如何O(1)删除元素？
// 通俗理解：这题重点是步骤化表达：先做什么、再做什么、为什么这么做。

#include <iostream>
#include <vector>

void erase_unordered(std::vector<int>& v, std::size_t idx) {
    v[idx] = v.back();
    v.pop_back();
}

int main() {
    std::vector<int> v{10, 20, 30, 40};
    erase_unordered(v, 1); // 不保序，O(1)
    std::cout << v.size() << " " << v[1] << "\n";
}


