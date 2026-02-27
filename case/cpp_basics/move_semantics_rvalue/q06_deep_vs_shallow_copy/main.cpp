// 题目：深拷贝和浅拷贝的区别？
// 通俗理解：先抓“它们各自解决什么问题”，再对比使用场景和代价。

#include <iostream>

struct Shallow {
    int* p;
    explicit Shallow(int v) : p(new int(v)) {}
    ~Shallow() { delete p; }
    // 默认拷贝是浅拷贝，这个类型如果复制会出问题（双删风险）
};

int main() {
    Shallow s(3);
    std::cout << *s.p << "\n";
}


