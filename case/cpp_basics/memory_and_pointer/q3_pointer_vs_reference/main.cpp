// 题目：指针和引用的区别？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>

void add_one_by_ptr(int* p) { if (p) ++(*p); }
void add_one_by_ref(int& r) { ++r; }

int main() {
    int x = 1;
    add_one_by_ptr(&x);
    add_one_by_ref(x);
    std::cout << x << "\n";
}


