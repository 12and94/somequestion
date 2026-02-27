// 题目：list和vector的区别？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>
#include <list>
#include <vector>

int main() {
    std::vector<int> v{1, 2, 3};
    std::list<int> l{1, 2, 3};
    v.insert(v.begin() + 1, 99);
    auto it = l.begin();
    std::advance(it, 1);
    l.insert(it, 99);
    std::cout << v[1] << " " << l.size() << "\n";
}


