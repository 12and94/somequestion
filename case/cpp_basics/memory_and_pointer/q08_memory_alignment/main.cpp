// 题目：内存对齐是什么？为什么要内存对齐？
// 通俗理解：先下定义，再给一个实际场景，面试会更有说服力。

#include <cstddef>
#include <iostream>

struct S {
    char c;
    int i;
};

int main() {
    std::cout << sizeof(S) << " " << offsetof(S, i) << "\n";
}


