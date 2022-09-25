#include "d_string.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

int scan_string(char** str, int* len, int fd) {
    errno = 0;
    int buf_size = 1;
    int new_len = 0;
    char *new_str = malloc(buf_size*sizeof(char));
    if (new_str == NULL)
        return -1;

    char c;
    int res_read;
    while ((res_read = read(fd, &c, sizeof(char))) > 0 && c != '\n') {
        new_str[new_len++] = c;
        if (new_len >= buf_size) {
            buf_size *= 2;
            new_str = realloc(new_str, buf_size*sizeof(char));
            if (new_str == NULL)
                return -1;
        }
    }

    new_str[new_len] = '\0';
    if (res_read == -1)
        return errno;
    free(*str);
    *str = new_str;
    *len = new_len;
    if (res_read == 0)
        return EOF;
    return 0;
}