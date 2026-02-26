#include <cstdlib>
#include <iostream>

struct Item {
    Item() { std::cout << "ctor\n"; }
    ~Item() { std::cout << "dtor\n"; }
};

int main() {
    Item* p = new Item();
    delete p;

    int* n = static_cast<int*>(std::malloc(sizeof(int)));
    *n = 123;
    std::cout << *n << "\n";
    std::free(n);
}
