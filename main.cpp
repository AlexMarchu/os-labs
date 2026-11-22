#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <atomic>
#include <cstdint>
#include "platform.h"
#include "log.h"

namespace {
const char* kShmName       = "logger_lab3_state";
const char* kMutexName     = "logger_lab3_mutex";
const char* kLeaderName    = "logger_lab3_leader";
}

// ---------------------------------------------------------------------------
// Режим "копия"
// ---------------------------------------------------------------------------
static int run_copy(const std::string& role) {
    Log log("logger.log");

    platform::SharedRegion region;
    bool created = false;
    if (!region.open(kShmName, sizeof(platform::SharedState), created)) {
        std::cerr << "copy: cannot open shm" << std::endl;
        return 1;
    }
    auto* state = static_cast<platform::SharedState*>(region.data());

    platform::SharedMutex mtx;
#if defined(PLATFORM_POSIX)
    void* mtx_ptr = &state->mutex;
#else
    void* mtx_ptr = nullptr;
#endif
    if (!mtx.init(mtx_ptr, created, kMutexName)) return 1;

    // helper: прочитать counter под mutex
    auto read_counter = [&]() -> std::int64_t {
        mtx.lock();
        auto v = state->counter;
        mtx.unlock();
        return v;
    };

    if (role == "--copy1") {
        log.write("copy1 started, counter=" + std::to_string(read_counter()));

        mtx.lock();
        state->counter += 10;
        auto new_val = state->counter;
        mtx.unlock();

        log.write("copy1 incremented counter by 10 -> " + std::to_string(new_val));
        log.write("copy1 exiting");
        return 0;
    }

    if (role == "--copy2") {
        log.write("copy2 started, counter=" + std::to_string(read_counter()));

        mtx.lock();
        state->counter *= 2;
        auto doubled = state->counter;
        mtx.unlock();
        log.write("copy2 doubled counter -> " + std::to_string(doubled));

        platform::sleep_ms(2000);

        mtx.lock();
        state->counter /= 2;
        auto halved = state->counter;
        mtx.unlock();
        log.write("copy2 halved counter -> " + std::to_string(halved));

        log.write("copy2 exiting");
        return 0;
    }

    log.write("copy: unknown role " + role);
    return 2;
}

// ---------------------------------------------------------------------------
// Основной процесс
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    // Копия?
    if (argc >= 2) {
        std::string a = argv[1];
        if (a == "--copy1" || a == "--copy2") {
            return run_copy(a);
        }
    }

    Log log("logger.log");
    if (!log.is_open()) {
        std::cerr << "Failed to open logger.log" << std::endl;
        return 1;
    }

    // --- shared region ---
    platform::SharedRegion region;
    bool created = false;
    if (!region.open(kShmName, sizeof(platform::SharedState), created)) {
        std::cerr << "Failed to open shared region" << std::endl;
        return 1;
    }
    auto* state = static_cast<platform::SharedState*>(region.data());
    if (created) state->counter = 0;

    platform::SharedMutex mtx;
#if defined(PLATFORM_POSIX)
    void* mtx_ptr = &state->mutex;
#else
    void* mtx_ptr = nullptr;
#endif
    if (!mtx.init(mtx_ptr, created, kMutexName)) return 1;

    // --- лидерство ---
    platform::LeaderLock leader;
    bool is_leader = leader.try_acquire(kLeaderName);

    // --- лог: пункт 1 лабы ---
    log.write("program started, is_leader=" + std::string(is_leader ? "yes" : "no"));

    // --- CLI-поток ---
    std::atomic<bool> quit{false};
    std::thread cli([&]{
        std::string line;
        while (!quit.load() && std::getline(std::cin, line)) {
            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;

            if (cmd == "q" || cmd == "quit" || cmd == "exit") {
                quit.store(true);
                break;
            }

            if (cmd == "set") {
                std::int64_t v = 0;
                if (!(iss >> v)) {
                    std::cout << "usage: set <number>\n";
                    continue;
                }
                mtx.lock();
                state->counter = v;
                mtx.unlock();
                log.write("counter set by user to " + std::to_string(v));
                continue;
            }

            if (!cmd.empty()) {
                std::cout << "unknown command: " << cmd << "\n";
            }
        }
    });

    std::cout << "Main pid=" << platform::get_pid()
              << " leader=" << (is_leader ? "yes" : "no")
              << "\nCommands: set <N>, q\n";

    // --- три таймера ---
    std::uint64_t next_300  = platform::now_ms() + 300;
    std::uint64_t next_1000 = platform::now_ms() + 1000;
    std::uint64_t next_3000 = platform::now_ms() + 3000;

    platform::ChildProcess child1, child2;

    while (!quit.load()) {
        platform::sleep_ms(10);
        std::uint64_t now = platform::now_ms();

        // ---- 300 мс: counter += 1 (все процессы!) ----
        if (now >= next_300) {
            next_300 += 300;
            mtx.lock();
            state->counter += 1;
            mtx.unlock();
        }

        // ---- 1000 мс: лидер пишет counter в лог ----
        if (now >= next_1000) {
            next_1000 += 1000;
            if (is_leader) {
                mtx.lock();
                auto v = state->counter;
                mtx.unlock();
                log.write("counter = " + std::to_string(v));
            }
        }

        // ---- 3000 мс: лидер порождает копии ----
        if (now >= next_3000) {
            next_3000 += 3000;
            if (is_leader) {
                int code = 0;

                // copy1: если предыдущая ещё жива — rejected, иначе spawn
                if (child1.try_reap(&code)) {
                    log.write("copy1 finished with code " + std::to_string(code));
                    child1.close();
                }
                if (child1.valid()) {
                    log.write("spawn copy1 rejected: previous still running");
                } else {
                    if (child1.spawn("--copy1"))
                        log.write("spawned copy1");
                    else
                        log.write("spawn copy1 failed");
                }

                // copy2: аналогично
                if (child2.try_reap(&code)) {
                    log.write("copy2 finished with code " + std::to_string(code));
                    child2.close();
                }
                if (child2.valid()) {
                    log.write("spawn copy2 rejected: previous still running");
                } else {
                    if (child2.spawn("--copy2"))
                        log.write("spawned copy2");
                    else
                        log.write("spawn copy2 failed");
                }
            }
        }
    }

    // --- завершение ---
    log.write("program exiting");
    if (is_leader) {
        leader.release();
    }

    // child.close() в деструкторах ChildProcess
    if (cli.joinable()) {
        // На Windows нет простого способа разбудить getline;
        // для лабы detach допустимо, процесс всё равно завершится.
        cli.detach();
    }

    return 0;
}