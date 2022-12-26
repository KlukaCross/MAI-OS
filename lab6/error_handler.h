#ifndef LAB6_ERROR_HANDLER_H
#define LAB6_ERROR_HANDLER_H
#include <stdbool.h>

void error_handler(bool res);
bool is_ok(char* message);
bool parent_not_found(char* message);
bool node_not_found(char* message);

#endif //LAB6_ERROR_HANDLER_H
