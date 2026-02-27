// 题目：移动赋值运算符怎么写？
// 通俗理解：这题重点是步骤化表达：先做什么、再做什么、为什么这么做。

#include <iostream>
#include <utility>

class Buffer {
public:
    explicit Buffer(int v = 0) : p_(new int(v)) {}
    Buffer(Buffer&& rhs) noexcept : p_(rhs.p_) { rhs.p_ = nullptr; }
    Buffer& operator=(Buffer&& rhs) noexcept {
        if (this == &rhs) return *this;
        delete p_;
        p_ = rhs.p_;
        rhs.p_ = nullptr;
        return *this;
    }
    ~Buffer() { delete p_; }
private:
    int* p_;
};

int main() {
    Buffer a(1), b(2);
    b = std::move(a);
    std::cout << "move assign done\n";
}


