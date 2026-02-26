#include <deque>
#include <iostream>

int main() {
    std::deque<int> d;
    d.push_front(1);
    d.push_back(2);
    d.push_front(0);
    std::cout << d.front() << " " << d.back() << "\n";
}
