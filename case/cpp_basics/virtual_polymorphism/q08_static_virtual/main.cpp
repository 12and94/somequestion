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
