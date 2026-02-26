#include <iostream>
#include <memory>

struct Node;
using NodePtr = std::shared_ptr<Node>;

struct Node {
    std::weak_ptr<Node> next; // 用 weak_ptr 打破循环
};

int main() {
    auto a = std::make_shared<Node>();
    auto b = std::make_shared<Node>();
    a->next = b;
    b->next = a;
    std::cout << a.use_count() << " " << b.use_count() << "\n";
}
