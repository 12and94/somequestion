#include <iostream>

struct Base {
    virtual ~Base() = default;
    virtual void run() { std::cout << "Base\n"; }
};

struct Derived final : Base {
    void run() override { std::cout << "Derived\n"; }
};

int main() {
    Derived d;
    d.run();
}
