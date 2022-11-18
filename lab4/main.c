#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "d_string.h"
#include "sync_constants.h"
#include <signal.h>
#include "print_error.h"

pthread_mutex_t MESSAGE_MUTEX;
pthread_mutex_t ERROR_MUTEX;

char* ERROR_PTR;

void accept_error_handler(int signum){
    pthread_mutex_lock(&ERROR_MUTEX);
    write(STDOUT_FILENO, ERROR_PTR, sizeof(char)*strlen(ERROR_PTR));
    pthread_mutex_unlock(&ERROR_MUTEX);
}

void send_chunk(char* msg_ptr, const char* st, int l, int r, pid_t child_pid) {
    pthread_mutex_lock(&MESSAGE_MUTEX);
    for (int i=l; i <= r; ++i) {
        msg_ptr[i] = st[i];
    }
    pthread_mutex_unlock(&MESSAGE_MUTEX);

    kill(child_pid, CHILD_SIGNAL_CHECK);
    // maybe sleep()?
}

int loop(char* msg_ptr, pid_t child_pid) {
    errno = 0;
    char* st = NULL;
    int st_len = 0;
    int scan_res;
    do {
       scan_res = scan_string(&st, &st_len, STDIN_FILENO);
       if (scan_res && scan_res != EOF)
           return errno;

       st[st_len] = '\n';
       for (int j=0; j < (st_len+MESSAGES_FILESIZE)/MESSAGES_FILESIZE; ++j) {
           int to = (j+1)*MESSAGES_FILESIZE > st_len? st_len : (j+1)*MESSAGES_FILESIZE;
           send_chunk(msg_ptr, st, j*MESSAGES_FILESIZE, to, child_pid);
       }

       if (scan_res == EOF && st_len == 0)
           break;

    } while (!scan_res);
    return errno;
}


int main() {
    errno = 0;
    char* filename = NULL;
    int filename_len = 0;
    int file_answers, file_messages, file_errors;
    if (scan_string(&filename, &filename_len, STDIN_FILENO)) {
        if (errno)
            return print_error("scan string error");
        return -1;
    }
    if ((file_answers = creat(filename, S_IRWXU)) == -1)
        return print_error("open file answers error");
    if ((file_messages = open(MESSAGES_FILENAME, O_RDWR|O_TRUNC|O_CREAT, S_IRWXU)) == -1)
        return print_error("open file messages error");
    if ((file_errors = open(ERRORS_FILENAME, O_RDWR|O_TRUNC|O_CREAT, S_IRWXU)) == -1)
        return print_error("open file errors error");

    pthread_mutexattr_t message_mutex_attr;
    pthread_mutexattr_t error_mutex_attr;

    pthread_mutexattr_init(&message_mutex_attr);
    pthread_mutexattr_init(&error_mutex_attr);

    pthread_mutexattr_setpshared(&message_mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setpshared(&error_mutex_attr, PTHREAD_PROCESS_SHARED);

    if (pthread_mutex_init(&MESSAGE_MUTEX, &message_mutex_attr))
        return print_error("message mutex init error");
    if (pthread_mutex_init(&ERROR_MUTEX, &error_mutex_attr))
        return print_error("error mutex init error");

    sync_struct sync_st = {MESSAGE_MUTEX, ERROR_MUTEX, getpid()};

    if (write(file_messages, &sync_st, sizeof(sync_struct)) == -1)
        return print_error("write struct error");

    pid_t id = fork();
    if (id < 0)
        return print_error("fork error");
    else if (id == 0) {
        if (dup2(file_answers, STDOUT_FILENO) == -1)
            return print_error("dup2 stdout error");
        execl("./child", "./child", (char *) NULL);
        return print_error("execl error");
    }

    signal(PARENT_SIGNAL_CHECK, accept_error_handler);
    if (errno)
        return print_error("signal register error");

    char *msg_ptr = mmap(NULL, MESSAGES_FILESIZE*sizeof(char),
                         PROT_WRITE, MAP_SHARED, file_messages, 0);
    if (errno)
        return print_error("message mmap error");
    ERROR_PTR = mmap(NULL, ERRORS_FILESIZE*sizeof(char),
                     PROT_READ, MAP_SHARED, file_errors, 0);
    if (errno)
        return print_error("error mmap error");

    if (close(file_answers))
        return print_error("close file answers error");
    else if (loop(msg_ptr, id))
        return print_error("loop error");
    int status;
    waitpid(id, &status, 0);
    return status;
}
