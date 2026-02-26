#include <iostream>

class DeepCopy {
public:
    DeepCopy(int v) : p_(new int(v)) {}
    DeepCopy(const DeepCopy& rhs) : p_(new int(*rhs.p_)) {} // 自定义深拷贝
    ~DeepCopy() { delete p_; }
private:
    int* p_;
};

int main() {
    DeepCopy a(7);
    DeepCopy b = a;
    std::cout << "deep copy done\n";
}
