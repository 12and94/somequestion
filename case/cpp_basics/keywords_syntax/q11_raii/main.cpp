// 题目：RAII是什么？
// 通俗理解：先下定义，再给一个实际场景，面试会更有说服力。

#include <iostream>
#include <memory>

int main() {
    // unique_ptr 通过析构自动释放资源，体现 RAII。
    auto p = std::make_unique<int>(7);
    std::cout << *p << "\n";
}


