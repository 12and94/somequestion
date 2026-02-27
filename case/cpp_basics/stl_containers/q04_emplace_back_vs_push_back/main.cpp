// 题目：emplace_back和push_back的区别？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::vector<std::pair<int, std::string>> v;
    v.push_back(std::make_pair(1, "a"));
    v.emplace_back(2, "b");
    std::cout << v.size() << "\n";
}


