// 题目：多态是什么？如何实现？静态多态和动态多态的区别？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>

int add(int a, int b) { return a + b; } // 静态多态可用重载/模板，这里先用普通函数对比

template <typename T>
T twice(T x) { return x + x; }

struct Animal {
    virtual ~Animal() = default;
    virtual void speak() const { std::cout << "Animal\n"; }
};
struct Dog : Animal {
    void speak() const override { std::cout << "Dog\n"; }
};

int main() {
    std::cout << twice(3) << " " << add(1, 2) << "\n"; // 编译期确定
    Animal* a = new Dog();
    a->speak(); // 运行期确定
    delete a;
}


