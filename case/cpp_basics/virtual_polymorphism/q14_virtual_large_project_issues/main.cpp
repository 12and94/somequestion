// 题目：虚函数在大型项目中有什么问题？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

#include <iostream>
#include <vector>

struct ITask {
    virtual ~ITask() = default;
    virtual int run(int x) const = 0;
};

struct AddOne : ITask {
    int run(int x) const override { return x + 1; }
};

int main() {
    // 热点路径里大量虚调用会影响优化；这里仅演示调用形态。
    std::vector<ITask*> tasks;
    AddOne t;
    tasks.push_back(&t);
    std::cout << tasks[0]->run(10) << "\n";
}


