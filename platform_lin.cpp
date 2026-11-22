#include "platform.h"
#include <unistd.h>
#include <ctime>
#include <cstdio>
#include <string>

namespace platform {

int get_pid() { return static_cast<int>(getpid()); }

std::uint64_t now_ms() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1000ull
         + static_cast<std::uint64_t>(ts.tv_nsec) / 1000000ull;
}

void sleep_ms(std::uint32_t ms) {
    timespec ts{};
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, nullptr);
}

std::string now_datetime_str() {
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    std::tm tm_buf{};
    localtime_r(&ts.tv_sec, &tm_buf);
    char buf[32];
    std::snprintf(buf, sizeof(buf),
                  "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
                  tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
                  tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec,
                  static_cast<long>(ts.tv_nsec / 1000000L));
    return std::string(buf);
}

std::string hello() { return "Hello from POSIX/Linux build"; }

// ... SharedRegion и SharedMutex из Шага 3 — как я давал ...