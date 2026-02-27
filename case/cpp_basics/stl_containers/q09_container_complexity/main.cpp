// 题目：常见容器的插入/查找/删除复杂度？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

#include <iostream>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

int main() {
    std::vector<int> v;
    std::list<int> l;
    std::map<int, int> m;
    std::unordered_map<int, int> um;
    std::cout << v.size() + l.size() + m.size() + um.size() << "\n";
}


