// 题目：vector的底层实现？扩容机制？
// 通俗理解：先记最小实现流程，再补常见优化点和坑点。

#include <iostream>
#include <vector>

int main() {
    std::vector<int> v;
    std::size_t last_cap = v.capacity();
    for (int i = 0; i < 16; ++i) {
        v.push_back(i);
        if (v.capacity() != last_cap) {
            std::cout << "cap=" << v.capacity() << "\n";
            last_cap = v.capacity();
        }
    }
}


