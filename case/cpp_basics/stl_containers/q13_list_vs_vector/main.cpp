#include <iostream>
#include <list>
#include <vector>

int main() {
    std::vector<int> v{1, 2, 3};
    std::list<int> l{1, 2, 3};
    v.insert(v.begin() + 1, 99);
    auto it = l.begin();
    std::advance(it, 1);
    l.insert(it, 99);
    std::cout << v[1] << " " << l.size() << "\n";
}
