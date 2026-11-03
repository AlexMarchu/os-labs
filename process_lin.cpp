#include "process_lib.h"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <sstream>

struct ProcessContext {
    pid_t pid;

    ProcessContext() : pid(-1) {}
};

BackgroundProcess::BackgroundProcess() {
    ctx = new ProcessContext();
}

BackgroundProcess::~BackgroundProcess() {
    delete ctx;
}

bool BackgroundProcess::start(const std::string& command) {
    std::istringstream iss(command);
    std::string arg;
    std::vector<std::string> args_str;
    while (iss >> arg) {
        args_str.push_back(arg);
    }

    if (args_str.empty()) {
        std::cout << "Command cannot be empty!" << std::endl;
        return false;
    }

    std::vector<char*> args;
    for (auto& s : args_str) {
        args.push_back(const_cast<char*>(s.c_str()));
    }
    args.push_back(nullptr);

    pid_t pid = fork();

    if (pid == 0) {
        execvp(args[0], args.data());
        exit(127);
    } else if (pid > 0) {
        ctx->pid = pid;
        return true;
    } else {
        std::cout << "Error calling fork!";
        return false;
    }
}

bool BackgroundProcess::wait(int& exit_code) {
    if (ctx->pid <= 0)
        return false;

    int status;
    if (waitpid(ctx->pid, &status, 0) == -1)
        return false;

    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
        return true;
    }

    return false;
}