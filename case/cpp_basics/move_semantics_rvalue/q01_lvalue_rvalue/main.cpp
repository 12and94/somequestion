// 题目：左值和右值的区别？右值引用是什么？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>
#include <string>

std::string make_name() { return "codex"; }

int main() {
    std::string a = "hello";      // 左值对象
    std::string b = make_name();   // 右值初始化
    std::string&& rr = std::move(a); // 右值引用
    std::cout << b << " " << rr.size() << "\n";
}


