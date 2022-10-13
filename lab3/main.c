#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "args_parsing.h"
#include "constants.h"
#include "string_utils.h"

typedef struct {
    long max_memory;
    long double res;
    long count;
} NW_args;

FILE *NUMBER_FILE = NULL;
pthread_mutex_t FILE_MUTEX = PTHREAD_MUTEX_INITIALIZER;

void *number_worker(void* args);

int main(int argc, char* argv[]) {
    char* filename = NULL;
    long MAX_THREADS = DEFAULT_MAX_THREADS;
    long MAX_MEMORY = DEFAULT_MAX_MEMORY;

    if (args_parsing(argc, argv, &filename, &MAX_THREADS, &MAX_MEMORY)) {
        print_help(argv[0]);
        return -1;
    }

    if ((NUMBER_FILE = fopen(filename, "r")) == NULL) {
        printf("file open error");
        return -1;
    }


    pthread_t threads[MAX_THREADS];
    NW_args threads_args[MAX_THREADS];
    long thread_memory = (MAX_MEMORY/MAX_THREADS) - ((MAX_MEMORY/MAX_THREADS) % (MAX_128_HEX_BYTES+1));
    if (thread_memory == 0) {
        printf("Not enough memory. Increase memory or decrease the number of threads");
        return -1;
    }
    for (int i = 0; i < MAX_THREADS; ++i) {
        threads_args[i].max_memory = thread_memory;
        threads_args[i].res = 0;
        threads_args[i].count = 0;
        pthread_create(&(threads[i]), NULL, number_worker, &(threads_args[i]));
    }

    for (int i = 0; i < MAX_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    long double result = 0;
    long count = 0;
    for (int i = 0; i < MAX_THREADS; ++i) {
        count += threads_args[i].count;
    }
    for (int i = 0; i < MAX_THREADS; ++i) {
        result += threads_args[i].res * (threads_args[i].count/(long double)count);
    }
    printf("%Lf", result);
    return 0;
}

void *number_worker(void* args) {
    NW_args *nw = args;
    char *char_buf = malloc(nw->max_memory);
    while (true) {
        pthread_mutex_lock(&FILE_MUTEX);
        int read_count = fread(char_buf, sizeof(char), nw->max_memory, NUMBER_FILE);
        pthread_mutex_unlock(&FILE_MUTEX);
        for (int i = 0; i < read_count/(MAX_128_HEX_BYTES+1); ++i) {
            unsigned __int128 tmp = hex_to_int128(char_buf+(MAX_128_HEX_BYTES+1)*i);
            (nw->count)++;
            nw->res = (nw->res) * (1 - 1 / (long double)(nw->count)) + (long double)tmp / (nw->count);
        }

        if (read_count < nw->max_memory)
            break;
    }
    return 0;
}