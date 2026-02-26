#include <iostream>
#include <memory>

int main() {
    auto up = std::make_unique<int>(10);
    auto sp1 = std::make_shared<int>(20);
    auto sp2 = sp1;
    std::weak_ptr<int> wp = sp1;

    std::cout << *up << " " << sp1.use_count() << "\n";
    sp1.reset();
    sp2.reset();
    std::cout << std::boolalpha << wp.expired() << "\n";
}
