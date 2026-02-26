#include <iostream>
#include <memory>
#include <thread>

int main() {
    auto sp = std::make_shared<int>(1);
    std::thread t1([sp] { std::cout << *sp << "\n"; });
    std::thread t2([sp] { std::cout << sp.use_count() << "\n"; });
    t1.join();
    t2.join();
}
