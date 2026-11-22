#include "log.h"
#include "platform.h"

#include <cstdio>
#include <utility>

Log::Log(const std::string& path)
    : file_(path, std::ios::out | std::ios::app) {}

Log::~Log() {
    if (file_.is_open()) {
        file_.flush();
    }
}

bool Log::is_open() const {
    return file_.is_open();
}

void Log::write(const std::string& msg) {
    if (!file_.is_open()) return;

    std::lock_guard<std::mutex> lk(mtx_);
    file_ << platform::now_datetime_str()
          << " [pid=" << platform::get_pid() << "] "
          << msg << '\n';
    file_.flush();
}