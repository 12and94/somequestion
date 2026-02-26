#include <cstdlib>
#include <iostream>

int main() {
    // malloc 可能通过 brk/mmap 等系统机制向 OS 申请内存（实现相关）。
    void* p = std::malloc(64);
    std::cout << (p != nullptr) << "\n";
    std::free(p);
}
