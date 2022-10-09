cmake_minimum_required(VERSION 3.22)
project(lab3 C)

set(CMAKE_C_STANDARD 99)

add_executable(lab3 main.c args_parsing.c string_utils.c string_utils.h generate_hex128.c)
target_link_libraries(lab3 -lquadmath -lm)

add_executable(gen string_utils.c string_utils.h generate_hex128.c)
target_link_libraries(gen -lquadmath -lm)