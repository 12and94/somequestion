// 题目：类中静态变量什么时候初始化？
// 通俗理解：关键是时机：在生命周期哪一阶段发生、由谁触发。

#include <iostream>

class Config {
public:
    static int level;
};

int Config::level = 3;

int main() {
    std::cout << Config::level << "\n";
    Config::level = 5;
    std::cout << Config::level << "\n";
}


