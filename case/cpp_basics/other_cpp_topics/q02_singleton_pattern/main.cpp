#include <iostream>

class Logger {
public:
    static Logger& instance() {
        static Logger ins;
        return ins;
    }
    void log(int v) { std::cout << v << "\n"; }
private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

int main() {
    Logger::instance().log(7);
}
