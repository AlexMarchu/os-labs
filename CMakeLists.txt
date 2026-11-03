cmake_minimum_required(VERSION 3.10)
project(Lab2)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    set(PLATFORM_SRC process_win.cpp)
else()
    set(PLATFORM_SRC process_lin.cpp)
endif()

add_executable(test_app main.cpp ${PLATFORM_SRC})
