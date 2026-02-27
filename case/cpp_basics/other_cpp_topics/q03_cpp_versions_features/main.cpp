// 题目：C++各版本的特性？
// 通俗理解：先用一句话说明核心结论，再结合这段代码看最小示例。

#include <iostream>
#include <optional>
#include <string_view>

int main() {
    // C++17 示例：optional + string_view
    std::optional<int> v = 5;
    std::string_view name = "cpp17";
    std::cout << name << " " << v.value() << "\n";
}


