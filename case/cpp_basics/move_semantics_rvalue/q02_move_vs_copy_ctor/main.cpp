#include <iostream>
#include <vector>

int main() {
    std::vector<int> src(5, 7);
    std::vector<int> copy = src;           // 拷贝构造
    std::vector<int> moved = std::move(src); // 移动构造
    std::cout << copy.size() << " " << moved.size() << " " << src.size() << "\n";
}
