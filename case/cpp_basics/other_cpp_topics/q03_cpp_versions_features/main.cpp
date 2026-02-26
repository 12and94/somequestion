#include <iostream>
#include <optional>
#include <string_view>

int main() {
    // C++17 示例：optional + string_view
    std::optional<int> v = 5;
    std::string_view name = "cpp17";
    std::cout << name << " " << v.value() << "\n";
}
