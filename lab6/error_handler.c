#include "error_handler.h"
#include <stdlib.h>
#include <zmq.h>
#include <string.h>

void error_handler(bool res) {
    if (res) return;
    if (errno) {
        printf("%s\n", zmq_strerror(errno));
        exit(errno);
    }
    printf("unknown error\n");
    exit(-1);
}

bool is_ok(char* message) {
    return !strncmp(message, "OK", 2);
}

bool parent_not_found(char* message) {
    return !strcmp(message, "Error: Parent not found");
}

bool node_not_found(char* message) {
    return !strcmp(message, "Error: Node not found");
}