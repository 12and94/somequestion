#include <iostream>
#include <vector>

struct Bullet {
    int id = -1;
    void reset() { id = -1; }
};

int main() {
    // 最简对象池思路：预分配 + 空闲索引复用。
    std::vector<Bullet> pool(4);
    std::vector<int> free_list{0, 1, 2, 3};
    int idx = free_list.back();
    free_list.pop_back();
    pool[idx].id = 100;
    std::cout << pool[idx].id << "\n";
}
