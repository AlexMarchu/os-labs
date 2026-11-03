#ifndef PROCESS_LIB_H
#define PROCESS_LIB_H

#include <string>

struct ProcessContext;

class BackgroundProcess {
private:
    ProcessContext* ctx;

public:
    BackgroundProcess();
    ~BackgroundProcess();

    bool start(const std::string& command);

    bool wait(int& exit_code);
};

#endif