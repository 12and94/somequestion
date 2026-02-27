// 题目：内存泄漏是什么？如何检测和解决？
// 通俗理解：这题重点是步骤化表达：先做什么、再做什么、为什么这么做。

#include <iostream>
#include <memory>

int main() {
    // 泄漏反例：int* p = new int(1); // 忘记 delete
    // 正例：用智能指针托管资源。
    auto p = std::make_unique<int>(1);
    std::cout << *p << "\n";
}


