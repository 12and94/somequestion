#include <iostream>
#include <unordered_map>
#include <unordered_set>

int main() {
    std::unordered_set<int> s{1, 2, 3};
    std::unordered_map<int, int> m{{1, 10}};
    std::cout << s.count(2) << " " << m[1] << "\n";
}
