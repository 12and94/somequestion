#include <iostream>
#include <vector>

int main() {
    std::vector<int> v;
    std::size_t last_cap = v.capacity();
    for (int i = 0; i < 16; ++i) {
        v.push_back(i);
        if (v.capacity() != last_cap) {
            std::cout << "cap=" << v.capacity() << "\n";
            last_cap = v.capacity();
        }
    }
}
