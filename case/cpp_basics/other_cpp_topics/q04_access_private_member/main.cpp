// 题目：怎么访问类的private变量？
// 通俗理解：这题重点是步骤化表达：先做什么、再做什么、为什么这么做。

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


