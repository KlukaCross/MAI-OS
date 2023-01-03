cmake_minimum_required(VERSION 3.22)
project(KP)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(structs)

add_executable(server server.cpp)
add_executable(client client.cpp)

target_link_libraries(server fifo_pipe group_char user user_message)
target_link_libraries(client fifo_pipe)
