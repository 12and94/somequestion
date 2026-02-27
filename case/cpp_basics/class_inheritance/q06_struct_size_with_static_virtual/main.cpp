// 题目：struct加static变量/虚函数后大小变化？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

#include <iostream>

struct S1 { int x; };
struct S2 { int x; static int y; };
int S2::y = 0;
struct S3 { int x; virtual void f() {} };

int main() {
    // static 成员不在对象内，virtual 通常引入 vptr。
    std::cout << sizeof(S1) << " " << sizeof(S2) << " " << sizeof(S3) << "\n";
}


