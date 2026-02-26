#include <iostream>
#include <string>

std::string make_text() {
    std::string s = "rvo";
    return s; // 可能触发 NRVO；否则再考虑移动
}

int main() {
    std::string t = make_text();
    std::cout << t << "\n";
}
