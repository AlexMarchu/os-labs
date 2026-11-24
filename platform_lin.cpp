#include "platform.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <ctime>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>

#define PROC_SELF_EXE "/proc/self/exe"

namespace platform {

int get_pid() {
    return static_cast<int>(getpid());
}

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
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {}
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

std::string hello() {
    return "Hello from POSIX/Linux build";
}

struct SharedRegion::Impl {
    int         fd   = -1;
    void*       ptr  = nullptr;
    std::size_t sz   = 0;
    std::string name;
};

SharedRegion::SharedRegion() : impl_(new Impl()) {}

SharedRegion::~SharedRegion() {
    close();
    delete impl_;
}

bool SharedRegion::open(const std::string& name, std::size_t size, bool& created) {
    if (impl_->ptr) return true;

    std::string shm_name = "/" + name;

    int fd = ::shm_open(shm_name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);
    if (fd >= 0) {
        created = true;
        if (::ftruncate(fd, static_cast<off_t>(size)) != 0) {
            ::close(fd);
            ::shm_unlink(shm_name.c_str());
            return false;
        }
    } else if (errno == EEXIST) {
        created = false;
        fd = ::shm_open(shm_name.c_str(), O_RDWR, 0666);
        if (fd < 0) return false;
    } else {
        return false;
    }

    void* p = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        ::close(fd);
        if (created) ::shm_unlink(shm_name.c_str());
        return false;
    }

    impl_->fd   = fd;
    impl_->ptr  = p;
    impl_->sz   = size;
    impl_->name = shm_name;
    return true;
}

void*       SharedRegion::data()       { return impl_->ptr; }
const void* SharedRegion::data() const { return impl_->ptr; }

void SharedRegion::close() {
    if (impl_->ptr) {
        ::munmap(impl_->ptr, impl_->sz);
        impl_->ptr = nullptr;
    }
    if (impl_->fd >= 0) {
        ::close(impl_->fd);
        impl_->fd = -1;
    }
}

void SharedRegion::unlink(const std::string& name) {
    ::shm_unlink(("/" + name).c_str());
}

struct SharedMutex::Impl {
    pthread_mutex_t* pmtx = nullptr;
};

SharedMutex::SharedMutex() : impl_(new Impl()) {}

SharedMutex::~SharedMutex() {
    delete impl_;
}

bool SharedMutex::init(void* shm_ptr, bool created, const std::string& /*name*/) {
    if (!shm_ptr) return false;

    impl_->pmtx = reinterpret_cast<pthread_mutex_t*>(shm_ptr);

    if (created) {
        pthread_mutexattr_t attr;
        if (pthread_mutexattr_init(&attr) != 0) return false;
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
        int rc = pthread_mutex_init(impl_->pmtx, &attr);
        pthread_mutexattr_destroy(&attr);
        if (rc != 0) return false;
    }
    return true;
}

void SharedMutex::lock() {
    int rc = pthread_mutex_lock(impl_->pmtx);
    if (rc == EOWNERDEAD) {
        pthread_mutex_consistent(impl_->pmtx);
    }
}

void SharedMutex::unlock() {
    pthread_mutex_unlock(impl_->pmtx);
}

#include <semaphore.h>

struct LeaderLock::Impl {
    sem_t* sem       = nullptr;
    bool   is_leader = false;
    std::string name;
};

LeaderLock::LeaderLock() : impl_(new Impl()) {}

LeaderLock::~LeaderLock() {
    release();
    delete impl_;
}

bool LeaderLock::try_acquire(const std::string& name) {
    if (impl_->sem) return impl_->is_leader;

    std::string sem_name = "/" + name;

    // если объект уже есть - мы не лидер
    sem_t* s = ::sem_open(sem_name.c_str(), O_CREAT | O_EXCL, 0666, 1);
    if (s != SEM_FAILED) {
        impl_->sem       = s;
        impl_->is_leader = true;
        impl_->name      = sem_name;
        return true;
    }
    if (errno != EEXIST) {
        return false;
    }

    sem_t* s2 = ::sem_open(sem_name.c_str(), 0);
    if (s2 != SEM_FAILED) {
        ::sem_close(s2);
    }
    return false;
}

void LeaderLock::release() {
    if (impl_->sem) {
        ::sem_close(impl_->sem);
        if (impl_->is_leader) {
            ::sem_unlink(impl_->name.c_str());
        }
        impl_->sem       = nullptr;
        impl_->is_leader = false;
    }
}

bool LeaderLock::held() const {
    return impl_->is_leader;
}

struct ChildProcess::Impl {
    pid_t pid = -1;
};

ChildProcess::ChildProcess() : impl_(new Impl()) {}
ChildProcess::~ChildProcess() {
    close();
    delete impl_;
}

bool ChildProcess::spawn(const std::string& arg) {
    if (impl_->pid > 0) return false;

    char exe_path[4096];
    ssize_t n = ::readlink(PROC_SELF_EXE, exe_path, sizeof(exe_path) - 1);
    if (n <= 0) return false;
    exe_path[n] = '\0';

    pid_t pid = ::fork();
    if (pid < 0) {
        return false;
    }
    if (pid == 0) {
        // child
        char* argv[3];
        argv[0] = exe_path;
        argv[1] = const_cast<char*>(arg.c_str());
        argv[2] = nullptr;

        ::execv(exe_path, argv);
        ::_exit(127);
    }

    // parent
    impl_->pid = pid;
    return true;
}

bool ChildProcess::try_reap(int* exit_code) {
    if (impl_->pid <= 0) return false;

    int status = 0;
    pid_t r = ::waitpid(impl_->pid, &status, WNOHANG);
    if (r == 0) return false;
    if (r < 0)  return false;
    if (r != impl_->pid) return false;

    if (exit_code) {
        if (WIFEXITED(status)) {
            *exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            *exit_code = 128 + WTERMSIG(status);
        } else {
            *exit_code = -1;
        }
    }

    impl_->pid = -1;
    return true;
}

void ChildProcess::close() {
    if (impl_->pid > 0) {
        impl_->pid = -1;
    }
}

bool ChildProcess::valid() const {
    return impl_->pid > 0;
}

} // namespace platform