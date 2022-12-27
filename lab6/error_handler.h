#ifndef LAB6_ERROR_HANDLER_H
#define LAB6_ERROR_HANDLER_H
#include <stdbool.h>

void error_handler(bool res);
bool is_ok(char* message);
bool node_not_found(char* message);
const char* ERROR_NODE_NOT_FOUND = "Error: Node not found";

#endif //LAB6_ERROR_HANDLER_H
