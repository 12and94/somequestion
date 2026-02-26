#include <iostream>

struct Resource {
    explicit Resource(int id) : id(id) {}
    int id;
};

int main() {
    // 构造函数不能是 virtual。
    // 需要“按类型创建对象”时，通常用工厂函数替代虚构造。
    auto make_resource = [](int id) { return Resource(id); };
    Resource r = make_resource(7);
    std::cout << r.id << "\n";
}
