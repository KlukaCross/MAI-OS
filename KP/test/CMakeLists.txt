cmake_minimum_required(VERSION 3.22)
project(TEST)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpanel -lncurses")

add_executable(test test.cpp)