#include <iostream>

class Config {
public:
    static int level;
};

int Config::level = 3;

int main() {
    std::cout << Config::level << "\n";
    Config::level = 5;
    std::cout << Config::level << "\n";
}
