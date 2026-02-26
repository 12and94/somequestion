#include <iostream>

struct S1 { int x; };
struct S2 { int x; static int y; };
int S2::y = 0;
struct S3 { int x; virtual void f() {} };

int main() {
    // static 成员不在对象内，virtual 通常引入 vptr。
    std::cout << sizeof(S1) << " " << sizeof(S2) << " " << sizeof(S3) << "\n";
}
