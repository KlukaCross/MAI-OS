add_library(string_parser STATIC string_parser.cpp)
add_library(gen_uuid STATIC gen_uuid.cpp)

target_link_libraries(gen_uuid uuid)
