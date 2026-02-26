#include <iostream>
#include <memory>

int main() {
    auto a = std::shared_ptr<int>(new int(3));
    auto b = std::make_shared<int>(4);
    std::cout << *a << " " << *b << "\n";
}
