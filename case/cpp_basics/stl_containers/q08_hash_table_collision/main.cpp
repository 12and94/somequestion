#include <iostream>
#include <unordered_map>

struct BadHash {
    std::size_t operator()(int) const { return 1; } // 故意制造冲突
};

int main() {
    std::unordered_map<int, int, BadHash> m;
    for (int i = 0; i < 5; ++i) m[i] = i * 10;
    std::cout << m.size() << "\n";
}
