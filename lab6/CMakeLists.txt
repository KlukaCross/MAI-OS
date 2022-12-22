cmake_minimum_required(VERSION 3.22)
project(lab6 C)

set(CMAKE_C_STANDARD 99)

add_executable(lab6 control_node.c compute_node.c balanced_tree.c mq_node.c)
