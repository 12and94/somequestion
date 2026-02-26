#include <iostream>

void add_one_by_ptr(int* p) { if (p) ++(*p); }
void add_one_by_ref(int& r) { ++r; }

int main() {
    int x = 1;
    add_one_by_ptr(&x);
    add_one_by_ref(x);
    std::cout << x << "\n";
}
