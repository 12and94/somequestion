#include <iostream>

int main() {
    int* p = new int(7);
    delete p;
    p = nullptr; // 释放后立刻置空，避免野指针
    std::cout << (p == nullptr) << "\n";
}
