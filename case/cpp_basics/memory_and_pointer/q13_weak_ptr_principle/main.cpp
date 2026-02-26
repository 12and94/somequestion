#include <iostream>
#include <memory>

int main() {
    std::weak_ptr<int> wp;
    {
        auto sp = std::make_shared<int>(42);
        wp = sp;
        if (auto lock = wp.lock()) std::cout << *lock << "\n";
    }
    std::cout << std::boolalpha << wp.expired() << "\n";
}
