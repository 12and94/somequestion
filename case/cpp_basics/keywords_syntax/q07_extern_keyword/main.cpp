// 题目：extern关键字的作用？
// 通俗理解：回答时先说它解决的核心痛点，再说副作用或限制。

#include <iostream>

extern int g_value; // 声明：定义在别处
int g_value = 42;   // 这里给出定义

int main() {
    std::cout << g_value << "\n";
}


