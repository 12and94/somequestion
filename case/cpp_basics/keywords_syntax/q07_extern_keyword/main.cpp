#include <iostream>

extern int g_value; // 声明：定义在别处
int g_value = 42;   // 这里给出定义

int main() {
    std::cout << g_value << "\n";
}
