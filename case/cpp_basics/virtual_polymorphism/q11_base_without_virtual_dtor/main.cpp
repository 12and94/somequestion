// 题目：基类不使用虚析构函数会发生什么？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

#include <iostream>

struct SafeBase {
    virtual ~SafeBase() = default;
};

struct SafeDerived : SafeBase {
    ~SafeDerived() override { std::cout << "SafeDerived destroyed\n"; }
};

int main() {
    // 正确做法：会被多态 delete 的基类使用虚析构。
    SafeBase* p = new SafeDerived();
    delete p;

    // 反例（不要这么写）：非虚析构基类 delete 派生对象是未定义行为。
}


