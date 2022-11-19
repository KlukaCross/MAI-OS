#include "d_string.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

int scan_string(char** str, int* len) {
    errno = 0;
    int buf_size = 1;
    int new_len = 0;
    char *new_str = malloc(buf_size*sizeof(char));
    if (new_str == NULL)
        return -1;

    int c;
    while ((c = getchar()) > 0 && c != '\n') {
        new_str[new_len++] = (char)c;
        if (new_len >= buf_size) {
            buf_size *= 2;
            new_str = realloc(new_str, buf_size*sizeof(char));
            if (new_str == NULL)
                return -1;
        }
    }

    new_str[new_len] = '\0';
    if (errno)
        return errno;
    free(*str);
    *str = new_str;
    *len = new_len;
    if (c == EOF)
        return EOF;
    return 0;
}