// 题目：四种类型转换(static_cast等)的区别和使用场景？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>

struct Base { virtual ~Base() = default; };
struct Derived : Base { void hello() { std::cout << "hi\n"; } };

int main() {
    double d = 3.6;
    int i = static_cast<int>(d);
    Base* b = new Derived();
    if (auto* p = dynamic_cast<Derived*>(b)) p->hello();
    delete b;
    std::cout << i << "\n";
}


