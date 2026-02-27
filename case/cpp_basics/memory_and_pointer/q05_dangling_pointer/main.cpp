// 题目：什么是野指针？如何避免？
// 通俗理解：这题重点是步骤化表达：先做什么、再做什么、为什么这么做。

#include <iostream>

int main() {
    int* p = new int(7);
    delete p;
    p = nullptr; // 释放后立刻置空，避免野指针
    std::cout << (p == nullptr) << "\n";
}


