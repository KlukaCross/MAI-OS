#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "response.h"

#define FILENAME_SIZE 256
#define DESCRIPTOR_SIZE 2

int handler(int pipe_out, int pipe_in, int child_id) {
    getchar(); // fictional \n
    while (1) {
        int c = getchar();
        write(pipe_out, &c, sizeof(int));
        if (c == EOF) {
            int status;
            waitpid(child_id, &status, 0);
            return status;
        }
        if (c == '\n') {
            char res[RESPONSE_TEXT_SIZE];
            read(pipe_in, &res, RESPONSE_TEXT_SIZE * sizeof(char));
            if (strcmp(res, "OK"))
                printf("%s\n", res);
        }
    }
}

int main() {
    errno = 0;
    int fd1[2];
    int fd2[2];
    char filename[FILENAME_SIZE];
    scanf("%s", filename);

    int file_d = open(filename, O_WRONLY | O_CREAT, S_IRWXU);
    if (file_d < 0) {
        perror("open error");
        return -1;
    }

    pipe(fd1);
    pipe(fd2);
    pid_t id = fork();
    if (id < 0) {
        perror("fork error");
        return -1;
    } else if (id == 0) {
        char child_in[DESCRIPTOR_SIZE], child_err[DESCRIPTOR_SIZE], child_out[DESCRIPTOR_SIZE];
        sprintf(child_in, "%d", fd1[0]);
        sprintf(child_out, "%d", file_d);
        sprintf(child_err, "%d", fd2[1]);
        execl("./child.out", "./child.out", child_in, child_out, child_err, (char *) NULL);
        perror("execl error");
    }
    int res = handler(fd1[1], fd2[0], id);
    return res;
}
