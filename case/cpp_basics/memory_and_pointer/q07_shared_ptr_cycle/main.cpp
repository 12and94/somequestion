// 题目：shared_ptr的底层实现？循环引用如何解决？
// 通俗理解：先记最小实现流程，再补常见优化点和坑点。

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


