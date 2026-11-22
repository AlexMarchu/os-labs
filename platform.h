#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstdint>
#include <string>

#if defined(PLATFORM_POSIX)
#include <pthread.h>
#endif

namespace platform {

int  get_pid();
std::uint64_t now_ms();
void sleep_ms(std::uint32_t ms);
std::string now_datetime_str();

// ---- Shared memory + межпроцессный mutex ----

struct SharedState {
    std::int64_t counter;
    // позже: pid лидера, флаги занятости копий и т.д.
#if defined(PLATFORM_POSIX)
    pthread_mutex_t mutex;   // только для POSIX; на Windows не используется
#endif
};

class SharedRegion {
public:
    SharedRegion();
    ~SharedRegion();

    SharedRegion(const SharedRegion&)            = delete;
    SharedRegion& operator=(const SharedRegion&) = delete;

    bool open(const std::string& name, std::size_t size, bool& created);

    void*       data();
    const void* data() const;

    void close();
    void unlink(const std::string& name);

private:
    struct Impl;
    Impl* impl_;
};

class SharedMutex {
public:
    SharedMutex();
    ~SharedMutex();

    SharedMutex(const SharedMutex&)            = delete;
    SharedMutex& operator=(const SharedMutex&) = delete;

    bool init(void* shm_ptr, bool created, const std::string& name);

    void lock();
    void unlock();

private:
    struct Impl;
    Impl* impl_;
};

// ---- Дочерний процесс ----

class ChildProcess {
public:
    ChildProcess();
    ~ChildProcess();

    ChildProcess(const ChildProcess&)            = delete;
    ChildProcess& operator=(const ChildProcess&) = delete;

    bool spawn(const std::string& arg);
    bool try_reap(int* exit_code);
    void close();
    bool valid() const;

private:
    struct Impl;
    Impl* impl_;
};

std::string hello();

class LeaderLock {
public:
    LeaderLock();
    ~LeaderLock();
    LeaderLock(const LeaderLock&)            = delete;
    LeaderLock& operator=(const LeaderLock&) = delete;

    // true  — этот процесс стал лидером (объект создан им первым)
    // false — лидер уже есть (объект существовал) или ошибка
    bool try_acquire(const std::string& name);

    void release();
    bool held() const;

private:
    struct Impl;
    Impl* impl_;
};

} // namespace platform

#endif // PLATFORM_H