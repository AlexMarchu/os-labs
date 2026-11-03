#include <iostream>
#include "process_lib.h"

int main() {
    BackgroundProcess bg_proc;

    std::string cmd;
    #ifdef _WIN32
        cmd = "ping 127.0.0.1";
    #else
        cmd = "ping -c 4 127.0.0.1";
    #endif

    std::cout << "Starting background process (" << cmd << ")" << std::endl;

    if (bg_proc.start(cmd)) {
        std::cout << "Process started succesfully. Waiting for it to finish..." << std::endl;

        int exit_code = 127;
        if (bg_proc.wait(exit_code))
            std::cout << "Background process finished with exit code: " << exit_code << std::endl;
        else 
            std::cout << "Failed to get exit code :(" << std::endl;
    }
    else
        std::cout << "Failed to start process" << std::endl;
    
    return 0;
}