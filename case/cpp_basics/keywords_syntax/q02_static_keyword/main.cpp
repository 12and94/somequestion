// 题目：static关键字的作用？
// 通俗理解：回答时先说它解决的核心痛点，再说副作用或限制。

#include <iostream>

int next_id() {
    static int id = 0; // 只初始化一次
    return ++id;
}

int main() {
    std::cout << next_id() << " " << next_id() << "\n";
}


