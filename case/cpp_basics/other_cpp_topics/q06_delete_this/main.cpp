// 题目：delete this的使用场景？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

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


