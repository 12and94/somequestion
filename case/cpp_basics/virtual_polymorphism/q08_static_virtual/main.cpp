// 题目：静态函数能否声明为虚函数？
// 通俗理解：这题通常先给结论，再解释语言规则或机制原因。

#include <iostream>

struct Demo {
    static void info() { std::cout << "static function\n"; }
    virtual void run() { std::cout << "virtual function\n"; }
    virtual ~Demo() = default;
};

int main() {
    // static 无 this，不能声明为 virtual。
    Demo d;
    Demo::info();
    d.run();
}


