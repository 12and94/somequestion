#include <iostream>

class Counter {
public:
    int get() const { return value_; } // 通过公有接口访问 private
private:
    int value_ = 42;
};

int main() {
    Counter c;
    std::cout << c.get() << "\n";
}
