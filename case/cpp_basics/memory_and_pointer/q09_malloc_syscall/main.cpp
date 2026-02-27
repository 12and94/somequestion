// 题目：malloc的底层实现？调用了哪个系统调用？
// 通俗理解：先记最小实现流程，再补常见优化点和坑点。

#include <cstdlib>
#include <iostream>

int main() {
    // malloc 可能通过 brk/mmap 等系统机制向 OS 申请内存（实现相关）。
    void* p = std::malloc(64);
    std::cout << (p != nullptr) << "\n";
    std::free(p);
}


