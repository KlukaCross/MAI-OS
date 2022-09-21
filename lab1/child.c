#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "response.h"
#include <stdlib.h>  // for strtol
#define BUF_SIZE 2048

int is_upper(char ch) {
    return 'A' <= ch && ch <= 'Z';
}

int main(int argc, char* argv[]) {
    int d_in = strtol(argv[1], NULL, 10);
    FILE* f = fopen(argv[2], "w");
    int d_err = strtol(argv[3], NULL, 10);
    int now_ch;
    char st[BUF_SIZE] = "";
    int st_len = 0;
    while (1) {
        read(d_in, &now_ch, sizeof(int));
        st[st_len] = (now_ch == EOF)? '\n' : (char) now_ch;
        st[++st_len] = '\0';

        if (st[st_len-1] != '\n' && st[st_len-1] != EOF)
            continue;

        char res[RESPONSE_TEXT_SIZE] = "";
        if (!is_upper(st[0]))
            strcpy(res, "invalid string");
        else {
            fwrite(st, sizeof(char), st_len, f);
            strcpy(res, "OK");
        }
        write(d_err, &res, sizeof(char) * RESPONSE_TEXT_SIZE);
        if (st[st_len-1] == EOF) {
            fclose(f);
            return 0;
        }
        st[st_len=0] = '\0';
    }
}
