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
