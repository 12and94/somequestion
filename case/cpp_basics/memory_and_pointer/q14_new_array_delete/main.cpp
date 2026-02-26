#include <iostream>

struct Obj {
    Obj() { std::cout << "c "; }
    ~Obj() { std::cout << "d "; }
};

int main() {
    Obj* arr = new Obj[2];
    delete[] arr; // new[] 对应 delete[]
    std::cout << "\n";
}
