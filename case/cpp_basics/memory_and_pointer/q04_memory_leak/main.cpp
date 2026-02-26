#include <iostream>
#include <memory>

int main() {
    // 泄漏反例：int* p = new int(1); // 忘记 delete
    // 正例：用智能指针托管资源。
    auto p = std::make_unique<int>(1);
    std::cout << *p << "\n";
}
