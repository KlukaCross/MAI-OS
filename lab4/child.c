#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <malloc.h>
#include <stdlib.h>
#include "sync_constants.h"
#include "print_error.h"

sync_struct SYNC_ST;

char* MESSAGE_PTR;
char* ERROR_PTR;

const char* INVALID_STRING = "invalid string\n";

int LEN_READ_ST = 0;
int MAX_READ_LEN = 1;
char* READ_ST = NULL;


int is_upper(char ch) {
    return 'A' <= ch && ch <= 'Z';
}

void read_chunk(int signum) {
    if (READ_ST == NULL)
        READ_ST = malloc(MAX_READ_LEN*sizeof(char));

    pthread_mutex_lock(&SYNC_ST.MESSAGE_MUTEX);

    int i=0;
    for (; MESSAGE_PTR[i] && MESSAGE_PTR[i] != '\n' && MESSAGE_PTR[i] != EOF; ++i) {
        READ_ST[LEN_READ_ST++] = MESSAGE_PTR[i];
        if (LEN_READ_ST >= MAX_READ_LEN) {
            MAX_READ_LEN *= 2;
            if ((READ_ST = realloc(READ_ST, MAX_READ_LEN*sizeof(char))) == NULL) {
                print_error("realloc error");
                return;
            }
        }
    }
    char last = MESSAGE_PTR[i];
    pthread_mutex_unlock(&SYNC_ST.MESSAGE_MUTEX);
    if ((last == '\n' || last == EOF) && i != 0) {
        READ_ST[i] = '\0';
        if (is_upper(READ_ST[0])) {
            if (printf("%s\n", READ_ST) == -1)
                print_error("printf error");
            if (fflush(STDIN_FILENO) == EOF)
                print_error("fflush error");
        } else {
            pthread_mutex_lock(&SYNC_ST.ERROR_MUTEX);
            strcpy(ERROR_PTR, INVALID_STRING);
            pthread_mutex_unlock(&SYNC_ST.ERROR_MUTEX);
            kill(SYNC_ST.parent_PID, PARENT_SIGNAL_CHECK);
        }
        LEN_READ_ST = 0;
    }
    if (last == EOF)
        exit(0);

}

int main() {
    errno = 0;

    int file_messages, file_errors;
    if ((file_messages = open(MESSAGES_FILENAME, O_RDWR)) == -1)
        return print_error("open file messages error");
    if ((file_errors = open(ERRORS_FILENAME, O_RDWR)) == -1)
        return print_error("open file errors error");

    if (read(file_messages, &SYNC_ST, sizeof(SYNC_ST)) == -1)
        return print_error("read sync struct error");

    MESSAGE_PTR = mmap(NULL, MESSAGES_FILESIZE*sizeof(char),
                         PROT_READ, MAP_SHARED, file_messages, 0);
    if (errno)
        return print_error("child message mmap error");
    ERROR_PTR = mmap(NULL, ERRORS_FILESIZE*sizeof(char),
                     PROT_WRITE, MAP_SHARED, file_errors, 0);
    if (errno)
        return print_error("child error mmap error");

    signal(CHILD_SIGNAL_CHECK, read_chunk);

    while (true);
}
