#include <iostream>
#include "process_lib.h"

int main() {
    BackgroundProcess bg_proc;

    std::cout << "Starting background process (ping 127.0.0.1)" << std::endl;

    std::string cmd = "ping 127.0.0.1";

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