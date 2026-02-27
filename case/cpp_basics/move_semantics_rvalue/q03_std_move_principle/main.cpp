// 题目：std::move的实现原理？作用是什么？
// 通俗理解：按“数据从哪里来、经过什么步骤、最后得到什么结果”来理解最稳。

#include <iostream>
#include <string>

int main() {
    std::string s = "abc";
    std::string t = std::move(s); // move 只做右值转换，真正搬资源在 string 的移动构造里
    std::cout << "t=" << t << ", s_size=" << s.size() << "\n";
}


