cmake_minimum_required(VERSION 3.22)
project(lab1 C)

set(CMAKE_C_STANDARD 99)

add_library(d_string STATIC d_string.c)
add_executable(main main.c)
add_executable(child child.c)

target_link_libraries(main d_string)
target_link_libraries(child d_string)
