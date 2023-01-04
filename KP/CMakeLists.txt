cmake_minimum_required(VERSION 3.22)
project(KP)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(structs)
add_subdirectory(funcs)

add_executable(server server.cpp)
add_executable(client client.cpp)

target_link_libraries(server fifo_pipe group_chat user user_message string_parser)
target_link_libraries(client fifo_pipe string_parser gen_uuid pthread)
