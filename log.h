#ifndef LOG_H
#define LOG_H

#include <fstream>
#include <mutex>
#include <string>

class Log {
public:
    explicit Log(const std::string& path);
    ~Log();

    bool is_open() const;

    // Пишет одну строку: "<datetime> [pid=<pid>] <msg>"
    void write(const std::string& msg);

private:
    std::ofstream file_;
    std::mutex    mtx_;
};

#endif // LOG_H