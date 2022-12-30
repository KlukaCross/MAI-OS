cmake_minimum_required(VERSION 3.22)
project(lab6 C)

set(CMAKE_C_STANDARD 99)

add_subdirectory(hashmap)
add_subdirectory(errors)
add_subdirectory(structures)

add_executable(control_node control_node.c)
add_executable(compute_node compute_node.c)

target_link_libraries(control_node balanced_tree mq_node rm_list error_handler hashmap zmq pthread uuid)
target_link_libraries(compute_node mq_node error_handler hashmap zmq)

