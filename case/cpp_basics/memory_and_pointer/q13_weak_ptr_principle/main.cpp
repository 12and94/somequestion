// 题目：weak_ptr的作用和实现原理？
// 通俗理解：按“数据从哪里来、经过什么步骤、最后得到什么结果”来理解最稳。

#include <iostream>
#include <memory>

int main() {
    std::weak_ptr<int> wp;
    {
        auto sp = std::make_shared<int>(42);
        wp = sp;
        if (auto lock = wp.lock()) std::cout << *lock << "\n";
    }
    std::cout << std::boolalpha << wp.expired() << "\n";
}


