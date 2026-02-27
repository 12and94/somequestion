// 题目：new和malloc的区别？delete和free的区别？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <cstdlib>
#include <iostream>

struct Item {
    Item() { std::cout << "ctor\n"; }
    ~Item() { std::cout << "dtor\n"; }
};

int main() {
    Item* p = new Item();
    delete p;

    int* n = static_cast<int*>(std::malloc(sizeof(int)));
    *n = 123;
    std::cout << *n << "\n";
    std::free(n);
}


