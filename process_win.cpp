#include "process_lib.h"
#include <iostream>
#include <windows.h>
#include <vector>

struct ProcessContext {
    HANDLE h_process;
    HANDLE h_thread;

    ProcessContext() : h_process(nullptr), h_thread(nullptr) {}
};

BackgroundProcess::BackgroundProcess() {
    ctx = new ProcessContext();
}

BackgroundProcess::~BackgroundProcess() {
    if (ctx->h_process)
        CloseHandle(ctx->h_process);
    if (ctx->h_thread)
        CloseHandle(ctx->h_thread);
    delete ctx;
}

bool BackgroundProcess::start(const std::string& command) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::vector<char> cmd_buffer(command.begin(), command.end());

    if (!CreateProcessA(
        NULL,
        cmd_buffer.data(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    )) {
        return false;
    }

    ctx->h_process = pi.hProcess;
    ctx->h_thread = pi.hThread;

    return true;
}

bool BackgroundProcess::wait(int& exit_code) {
    if (!ctx->h_process)
        return false;

    WaitForSingleObject(ctx->h_process, INFINITE);

    DWORD exit_code_dword;
    if (GetExitCodeProcess(ctx->h_process, &exit_code_dword)) {
        exit_code = static_cast<int>(exit_code_dword);
        return true;
    }

    return false;
}