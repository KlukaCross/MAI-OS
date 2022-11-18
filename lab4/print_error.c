#include "print_error.h"

int print_error(char* err) {
    perror(err);
    return errno;
}