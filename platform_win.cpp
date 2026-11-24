#include "platform.h"
#include <windows.h>
#include <cstdio>
#include <string>

namespace platform {

int get_pid() {
    return static_cast<int>(GetCurrentProcessId());
}

std::uint64_t now_ms() {
    return static_cast<std::uint64_t>(GetTickCount64());
}

void sleep_ms(std::uint32_t ms) {
    Sleep(ms);
}

std::string now_datetime_str() {
    SYSTEMTIME st;
    GetLocalTime(&st);

    char buf[32];
    std::snprintf(buf, sizeof(buf),
                  "%04u-%02u-%02u %02u:%02u:%02u.%03u",
                  static_cast<unsigned>(st.wYear),
                  static_cast<unsigned>(st.wMonth),
                  static_cast<unsigned>(st.wDay),
                  static_cast<unsigned>(st.wHour),
                  static_cast<unsigned>(st.wMinute),
                  static_cast<unsigned>(st.wSecond),
                  static_cast<unsigned>(st.wMilliseconds));
    return std::string(buf);
}

std::string hello() {
    return "Hello from Windows build";
}

struct SharedRegion::Impl {
    HANDLE h_map   = nullptr;
    void*  ptr     = nullptr;
    std::size_t sz = 0;
};

SharedRegion::SharedRegion() : impl_(new Impl()) {}

SharedRegion::~SharedRegion() {
    close();
    delete impl_;
}

bool SharedRegion::open(const std::string& name, std::size_t size, bool& created) {
    if (impl_->ptr) return true; // уже открыт

    std::string full_name = "Local\\" + name;

    HANDLE h = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0, static_cast<DWORD>(size),
        full_name.c_str());

    if (!h) return false;

    created = (GetLastError() != ERROR_ALREADY_EXISTS);

    void* p = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (!p) {
        CloseHandle(h);
        return false;
    }

    impl_->h_map = h;
    impl_->ptr   = p;
    impl_->sz    = size;
    return true;
}

void*       SharedRegion::data()       { return impl_->ptr; }
const void* SharedRegion::data() const { return impl_->ptr; }

void SharedRegion::close() {
    if (impl_->ptr)   { UnmapViewOfFile(impl_->ptr); impl_->ptr = nullptr; }
    if (impl_->h_map) { CloseHandle(impl_->h_map);   impl_->h_map = nullptr; }
    impl_->sz = 0;
}

void SharedRegion::unlink(const std::string&) {}

struct SharedMutex::Impl {
    HANDLE h_mutex = nullptr;
};

SharedMutex::SharedMutex() : impl_(new Impl()) {}

SharedMutex::~SharedMutex() {
    if (impl_->h_mutex) {
        CloseHandle(impl_->h_mutex);
        impl_->h_mutex = nullptr;
    }
    delete impl_;
}

bool SharedMutex::init(void* /*shm_ptr*/, bool /*created*/, const std::string& name) {
    std::string full_name = "Local\\" + name;
    HANDLE h = CreateMutexA(nullptr, FALSE, full_name.c_str());
    if (!h) return false;
    impl_->h_mutex = h;
    return true;
}

void SharedMutex::lock() {
    WaitForSingleObject(impl_->h_mutex, INFINITE);
}

void SharedMutex::unlock() {
    ReleaseMutex(impl_->h_mutex);
}

struct ChildProcess::Impl {
    HANDLE h_process = nullptr;
};

ChildProcess::ChildProcess() : impl_(new Impl()) {}

ChildProcess::~ChildProcess() {
    close();
    delete impl_;
}

bool ChildProcess::spawn(const std::string& arg) {
    if (impl_->h_process) {
        return false;
    }

    char exe_path[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
    if (n == 0 || n == MAX_PATH) return false;

    std::string cmdline = std::string("\"") + exe_path + "\" " + arg;

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    if (!CreateProcessA(
            nullptr,
            cmdline.data(),
            nullptr, nullptr,
            FALSE, 
            0, nullptr, nullptr,
            &si, &pi)) {
        return false;
    }

    impl_->h_process = pi.hProcess;
    CloseHandle(pi.hThread);
    return true;
}

bool ChildProcess::try_reap(int* exit_code) {
    if (!impl_->h_process) return false;

    DWORD wr = WaitForSingleObject(impl_->h_process, 0);
    if (wr == WAIT_TIMEOUT) {
        return false;
    }
    if (wr != WAIT_OBJECT_0) {
        return false;
    }

    if (exit_code) {
        DWORD code = 0;
        if (!GetExitCodeProcess(impl_->h_process, &code)) return false;
        *exit_code = static_cast<int>(code);
    }

    CloseHandle(impl_->h_process);
    impl_->h_process = nullptr;
    return true;
}

void ChildProcess::close() {
    if (impl_->h_process) {
        CloseHandle(impl_->h_process);
        impl_->h_process = nullptr;
    }
}

bool ChildProcess::valid() const {
    return impl_->h_process != nullptr;
}

struct LeaderLock::Impl {
    HANDLE h = nullptr;
};

LeaderLock::LeaderLock() : impl_(new Impl()) {}

LeaderLock::~LeaderLock() {
    release();
    delete impl_;
}

bool LeaderLock::try_acquire(const std::string& name) {
    if (impl_->h) return true;

    std::string full_name = "Local\\" + name;

    HANDLE h = CreateMutexA(nullptr, TRUE, full_name.c_str());
    if (!h) return false;

    bool already_exists = (GetLastError() == ERROR_ALREADY_EXISTS);
    if (already_exists) {
        CloseHandle(h);
        return false;
    }

    impl_->h = h;
    return true;
}

void LeaderLock::release() {
    if (impl_->h) {
        ReleaseMutex(impl_->h);
        CloseHandle(impl_->h);
        impl_->h = nullptr;
    }
}

bool LeaderLock::held() const {
    return impl_->h != nullptr;
}

} // namespace platform

