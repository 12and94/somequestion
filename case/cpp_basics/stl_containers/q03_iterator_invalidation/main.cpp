#include <iostream>
#include <vector>

int main() {
    std::vector<int> v{1, 2, 3, 4, 5};
    for (auto it = v.begin(); it != v.end();) {
        if (*it % 2 == 0) it = v.erase(it); // 用返回值避免失效迭代器
        else ++it;
    }
    std::cout << v.size() << "\n";
}
