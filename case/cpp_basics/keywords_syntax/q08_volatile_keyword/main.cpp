// 题目：volatile关键字的作用？
// 通俗理解：回答时先说它解决的核心痛点，再说副作用或限制。

#include <iostream>

volatile int g_flag = 0;

int main() {
    // volatile 让编译器每次都从内存读写该变量。
    g_flag = 1;
    std::cout << g_flag << "\n";
}


