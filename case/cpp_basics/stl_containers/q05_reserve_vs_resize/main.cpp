#include <iostream>
#include <vector>

int main() {
    std::vector<int> v;
    v.reserve(100);
    std::cout << "size=" << v.size() << ", cap=" << v.capacity() << "\n";
    v.resize(3);
    std::cout << "size=" << v.size() << ", cap=" << v.capacity() << "\n";
}
