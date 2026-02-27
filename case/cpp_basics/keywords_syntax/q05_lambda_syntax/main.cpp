// 题目：lambda表达式的语法和使用？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

#include <iostream>
#include <vector>

int main() {
    int bias = 3;
    auto plus_bias = [bias](int x) { return x + bias; };
    std::vector<int> v{1, 2, 3};
    std::cout << plus_bias(v[0]) << "\n";
}


