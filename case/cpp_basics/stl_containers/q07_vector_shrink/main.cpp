#include <iostream>
#include <vector>

int main() {
    std::vector<int> v(1000, 1);
    std::cout << v.capacity() << "\n";
    v.clear();
    std::vector<int>().swap(v); // 常见强制回收容量写法
    std::cout << v.capacity() << "\n";
}
