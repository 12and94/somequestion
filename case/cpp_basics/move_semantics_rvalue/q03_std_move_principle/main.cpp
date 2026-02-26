#include <iostream>
#include <string>

int main() {
    std::string s = "abc";
    std::string t = std::move(s); // move 只做右值转换，真正搬资源在 string 的移动构造里
    std::cout << "t=" << t << ", s_size=" << s.size() << "\n";
}
