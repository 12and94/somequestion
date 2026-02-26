#include <iostream>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

int main() {
    std::vector<int> v;
    std::list<int> l;
    std::map<int, int> m;
    std::unordered_map<int, int> um;
    std::cout << v.size() + l.size() + m.size() + um.size() << "\n";
}
