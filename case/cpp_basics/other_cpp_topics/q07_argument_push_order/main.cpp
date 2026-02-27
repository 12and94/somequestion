// 题目：函数实参的压栈顺序？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

#include <iostream>

void foo(int a, int b) {
    std::cout << a << " " << b << "\n";
}

int main() {
    int i = 0;
    // 不要依赖实参求值顺序：foo(i++, i++);
    foo(i + 1, i + 2);
}


