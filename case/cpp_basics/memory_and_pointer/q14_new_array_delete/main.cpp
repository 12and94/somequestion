// 题目：new[]的对象怎么删除？为什么？
// 通俗理解：这题重点是步骤化表达：先做什么、再做什么、为什么这么做。

#include <iostream>

struct Obj {
    Obj() { std::cout << "c "; }
    ~Obj() { std::cout << "d "; }
};

int main() {
    Obj* arr = new Obj[2];
    delete[] arr; // new[] 对应 delete[]
    std::cout << "\n";
}


