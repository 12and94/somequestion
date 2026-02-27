// 题目：智能指针new和make_shared的区别？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>
#include <memory>

int main() {
    auto a = std::shared_ptr<int>(new int(3));
    auto b = std::make_shared<int>(4);
    std::cout << *a << " " << *b << "\n";
}


