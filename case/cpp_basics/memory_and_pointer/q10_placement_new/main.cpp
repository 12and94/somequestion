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
