// 题目：deque的底层实现？
// 通俗理解：先记最小实现流程，再补常见优化点和坑点。

#include <deque>
#include <iostream>

int main() {
    std::deque<int> d;
    d.push_front(1);
    d.push_back(2);
    d.push_front(0);
    std::cout << d.front() << " " << d.back() << "\n";
}


