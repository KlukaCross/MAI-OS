#include "error_handler.h"
#include <stdlib.h>
#include <zmq.h>

void error_handler_zmq(bool res) {
    if (res) return;
    if (errno) {
        printf("zmq error: %s\n", zmq_strerror(errno));
        exit(errno);
    }
    printf("unknown error\n");
    exit(-1);
}

void error_handler(bool res, char* err) {
    if (res) return;
    printf("%s", err);
    if (errno) {
        perror("");
        exit(errno);
    }
    printf(": unknown error\n");
    exit(-1);
}
