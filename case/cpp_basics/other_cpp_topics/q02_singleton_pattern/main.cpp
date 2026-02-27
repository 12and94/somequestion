// 题目：单例模式怎么写？
// 通俗理解：这题重点是步骤化表达：先做什么、再做什么、为什么这么做。

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


