#include <iostream>
#include <map>
#include <unordered_map>

int main() {
    std::map<int, int> ordered{{2, 20}, {1, 10}};
    std::unordered_map<int, int> hashed{{2, 20}, {1, 10}};
    std::cout << ordered.begin()->first << " " << hashed.size() << "\n";
}
