cmake_minimum_required(VERSION 3.22)
project(lab4 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_library(d_string STATIC d_string.c)
add_library(sync_constants INTERFACE)
add_library(print_error STATIC print_error.c)
add_executable(main main.c)
add_executable(child child.c)

target_link_libraries(main d_string sync_constants print_error)
target_link_libraries(child d_string sync_constants print_error)
