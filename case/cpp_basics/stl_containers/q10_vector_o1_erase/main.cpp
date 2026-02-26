#include <iostream>
#include <vector>

void erase_unordered(std::vector<int>& v, std::size_t idx) {
    v[idx] = v.back();
    v.pop_back();
}

int main() {
    std::vector<int> v{10, 20, 30, 40};
    erase_unordered(v, 1); // 不保序，O(1)
    std::cout << v.size() << " " << v[1] << "\n";
}
