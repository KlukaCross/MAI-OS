#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include "d_string.h"

int loop(int pipe_out, int pipe_in) {
    errno = 0;
    char* st = NULL;
    int len = 0;
    char* end_of_string = "\n";
    int scan_res;
    do {
       scan_res = scan_string(&st, &len, STDIN_FILENO);
       if (scan_res && scan_res != EOF)
           return errno;

       if (write(pipe_out, st, len*sizeof(char)) == -1 ||
        write(pipe_out, end_of_string, sizeof(char)) == -1)
           return errno;

       if (scan_res == EOF && len == 0)
           break;

       int poll_res = poll(&(struct pollfd){.fd=pipe_in, .events=POLLIN}, 1, 1);
       if (poll_res == -1)
           return errno;
       if (poll_res == 1) {
           char* err_st = NULL;
           int len_st = 0;
           if (scan_string(&err_st, &len_st, pipe_in))
               return errno;
           printf("%s\n", err_st);
       }
    } while (!scan_res);
    return errno;
}

int main() {
    errno = 0;
    int fd1[2];
    int fd2[2];
    char* filename = NULL;
    int filename_len = 0;
    if (scan_string(&filename, &filename_len, STDIN_FILENO)) {
        if (errno) {
            perror("scan string error");
            return errno;
        }
        return -1;
    }
    int file_d = creat(filename, S_IRWXU);
    if (file_d < 0)
        perror("open error");
    else if (pipe(fd1))
        perror("pipe1 error");
    else if (pipe(fd2))
        perror("pipe2 error");
    if (errno)
        return errno;
    pid_t id = fork();
    if (id < 0) {
        perror("fork error");
        return errno;
    } else if (id == 0) {
        if (dup2(fd1[0], STDIN_FILENO) == -1)
            perror("dup2 stdin error");
        else if (dup2(file_d, STDOUT_FILENO) == -1)
            perror("dup2 stdout error");
        else if (dup2(fd2[1], STDERR_FILENO) == -1)
            perror("dup2 stderr error");
        else if (close(fd2[0]))
            perror("close pipe2 read error");
        else if (close(fd1[1]))
            perror("close pipe1 write error");
        else {
            execl("./child", "./child", (char *) NULL);
            perror("execl error");
        }
        return errno;
    }
    if (close(fd2[1]))
        perror("close pipe2 write error");
    else if (close(fd1[0]))
        perror("close pipe1 read error");
    else if (close(file_d))
        perror("close file write error");
    else if (loop(fd1[1], fd2[0]))
        perror("loop error");
    else if (close(fd2[0]))
        perror("close pipe2 read error");
    else if (close(fd1[1]))
        perror("close pipe1 write error");
    else {
        int status;
        waitpid(id, &status, 0);
        return status;
    }
    return errno;
}
