#ifndef LAB1_SYNC_CONSTANTS_H
#define LAB1_SYNC_CONSTANTS_H
#include <pthread.h>
#include <signal.h>

typedef struct {
    pthread_mutex_t MESSAGE_MUTEX;
    pthread_mutex_t ERROR_MUTEX;
    int parent_PID;
} sync_struct;

const char MESSAGES_FILENAME[] = "messages";
const char ERRORS_FILENAME[] = "errors";

const int CHILD_SIGNAL_CHECK = SIGUSR1;
const int PARENT_SIGNAL_CHECK = SIGUSR1;

const int MESSAGES_FILESIZE = 1024;
const int ERRORS_FILESIZE = 128;

#endif //LAB1_SYNC_CONSTANTS_H
