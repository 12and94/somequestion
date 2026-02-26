#include <iostream>

volatile int g_flag = 0;

int main() {
    // volatile 让编译器每次都从内存读写该变量。
    g_flag = 1;
    std::cout << g_flag << "\n";
}
