// 题目：placement new是什么？使用场景？
// 通俗理解：先下定义，再给一个实际场景，面试会更有说服力。

#include <iostream>
#include <new>

struct Obj {
    explicit Obj(int v) : v(v) {}
    int v;
};

int main() {
    alignas(Obj) unsigned char buf[sizeof(Obj)];
    Obj* p = new (buf) Obj(9); // 在给定内存上构造对象
    std::cout << p->v << "\n";
    p->~Obj();
}


