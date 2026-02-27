// 题目：哈希表的实现？哈希冲突如何解决？
// 通俗理解：先记最小实现流程，再补常见优化点和坑点。

#include <iostream>
#include <unordered_map>

struct BadHash {
    std::size_t operator()(int) const { return 1; } // 故意制造冲突
};

int main() {
    std::unordered_map<int, int, BadHash> m;
    for (int i = 0; i < 5; ++i) m[i] = i * 10;
    std::cout << m.size() << "\n";
}


