cmake_minimum_required(VERSION 3.10)
project(logger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(logger main.cpp log.cpp)

if(WIN32)
    target_compile_definitions(logger PRIVATE PLATFORM_WIN32)
    target_sources(logger PRIVATE platform_win.cpp)
else()
    target_compile_definitions(logger PRIVATE PLATFORM_POSIX)
    target_sources(logger PRIVATE platform_lin.cpp)
    find_package(Threads REQUIRED)
    target_link_libraries(logger PRIVATE Threads::Threads rt)
endif()