cmake_minimum_required(VERSION 3.10)
project(os)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    message(STATUS "Configuring for Windows...")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux...")
endif()

set(OS_SRC hello.cpp)

add_executable(${PROJECT_NAME} main.cpp ${OS_SRC})