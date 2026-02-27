// 题目：移动语义和RVO的区别？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>
#include <string>

std::string make_text() {
    std::string s = "rvo";
    return s; // 可能触发 NRVO；否则再考虑移动
}

int main() {
    std::string t = make_text();
    std::cout << t << "\n";
}


