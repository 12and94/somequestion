#include <iostream>

int next_id() {
    static int id = 0; // 只初始化一次
    return ++id;
}

int main() {
    std::cout << next_id() << " " << next_id() << "\n";
}
