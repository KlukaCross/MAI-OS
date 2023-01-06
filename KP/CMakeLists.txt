cmake_minimum_required(VERSION 3.22)
project(KP)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(structs)
add_subdirectory(funcs)
add_subdirectory(tui)

add_library(server STATIC server/server.cpp)
add_library(client STATIC client/client.cpp)

target_link_libraries(server fifo_pipe group_chat user user_message string_parser)
target_link_libraries(client fifo_pipe string_parser gen_uuid pthread)
target_link_libraries(tui panel ncurses text_buffer chat_buffer)

add_executable(messenger messenger.cpp)
add_executable(messenger_server messenger_server.cpp)

target_link_libraries(messenger client tui)
target_link_libraries(messenger_server server)
