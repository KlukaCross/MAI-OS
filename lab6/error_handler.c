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