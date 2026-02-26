#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::vector<std::pair<int, std::string>> v;
    v.push_back(std::make_pair(1, "a"));
    v.emplace_back(2, "b");
    std::cout << v.size() << "\n";
}
