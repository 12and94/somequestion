#include <iostream>
#include <memory>

int main() {
    // unique_ptr 通过析构自动释放资源，体现 RAII。
    auto p = std::make_unique<int>(7);
    std::cout << *p << "\n";
}
