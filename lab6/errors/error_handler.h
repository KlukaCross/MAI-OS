#ifndef LAB6_ERROR_HANDLER_H
#define LAB6_ERROR_HANDLER_H
#include <stdbool.h>
#define ERROR_NOT_FOUND "Not found"
#define CODE_OK 1
#define CODE_ERROR_NOT_FOUND 2
#define CODE_ERROR_CUSTOM 3

void error_handler_zmq(bool res);
void error_handler(bool res, char* err);

#endif //LAB6_ERROR_HANDLER_H
