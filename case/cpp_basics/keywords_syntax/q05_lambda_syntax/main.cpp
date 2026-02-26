#include <iostream>
#include <vector>

int main() {
    int bias = 3;
    auto plus_bias = [bias](int x) { return x + bias; };
    std::vector<int> v{1, 2, 3};
    std::cout << plus_bias(v[0]) << "\n";
}
