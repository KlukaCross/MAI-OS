#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "d_string.h"

int is_upper(char ch) {
    return 'A' <= ch && ch <= 'Z';
}

int main() {
    errno = 0;
    char* invalid_string = "invalid string\n";
    char* st = NULL;
    int len = 0;
    while (!scan_string(&st, &len, STDIN_FILENO)) {
        if (is_upper(st[0])) {
            if (printf("%s\n", st) == -1) {
                perror("printf error");
                return errno;
            }
        }
        else if (write(STDERR_FILENO, invalid_string,
                         strlen(invalid_string)*sizeof(char)) == -1) {
            perror("write error");
            return errno;
        }
    }
    return errno;
}
