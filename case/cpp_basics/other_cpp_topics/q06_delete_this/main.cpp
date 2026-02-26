#include <iostream>

class SelfDelete {
public:
    static SelfDelete* create() { return new SelfDelete(); }
    void destroy() { delete this; } // 仅演示语法，工程上高风险
private:
    SelfDelete() = default;
    ~SelfDelete() { std::cout << "destroyed\n"; }
};

int main() {
    SelfDelete* p = SelfDelete::create();
    p->destroy();
}
