#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <quadmath.h>
#include "args_parsing.h"
#include "constants.h"
#include "string_utils.h"

typedef struct {
    long max_memory;
    __float128* res;
    long* count;
} NW_args;

FILE *NUMBER_FILE;
pthread_mutex_t FILE_MUTEX = PTHREAD_MUTEX_INITIALIZER;

void *number_worker(void* args);

int main(int argc, char* argv[]) {
    char* filename = NULL;
    long MAX_THREADS = DEFAULT_MAX_THREADS;
    long MAX_MEMORY = DEFAULT_MAX_MEMORY;

    if (!args_parsing(argc, argv, &filename, &MAX_THREADS, &MAX_MEMORY)) {
        print_help(argv[0]);
        return -1;
    }

    NUMBER_FILE = fopen(filename, "r");

    pthread_t threads[MAX_THREADS];
    NW_args threads_args[MAX_THREADS];
    long thread_memory = MAX_MEMORY/MAX_THREADS; // (MAX_MEMORY-2*sizeof(__int128))/MAX_THREADS ?
    for (int i = 0; i < MAX_THREADS; ++i) {
        threads_args[i].max_memory = thread_memory;
        *threads_args[i].res = 0;
        pthread_create(&threads[i], NULL, number_worker, &threads_args[i]);
    }

    for (int i = 0; i < MAX_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    __float128 result = 0;
    long count = 0;
    for (int i = 0; i < MAX_THREADS; ++i) {
        result += (*threads_args[i].res) * (*threads_args[i].count);
        count += (*threads_args[i].count);
    }
    result = result / count;
    printf("");
    return 0;
}

void *number_worker(void* args) {
    NW_args *nw = args;
    char *char_buf = malloc(nw->max_memory);
    while (true) {
        pthread_mutex_lock(&FILE_MUTEX);
        int read_count = fread(char_buf, sizeof(char), nw->max_memory, NUMBER_FILE);
        int shift = read_count;
        if (read_count == nw->max_memory && !is_separator(char_buf[nw->max_memory-1])) {
            for (shift -= 1; shift > 0; --shift) {
                if (is_separator(char_buf[shift])) {
                    fseek(NUMBER_FILE, shift-read_count, SEEK_CUR);
                    break;
                }
            }
        }
        pthread_mutex_unlock(&FILE_MUTEX);
        while (shift > 0) {
            __float128 tmp = hex_to_float128(char_buf, &shift);
            *nw->res = (*nw->res) * (1 - 1 / (__float128)(*nw->count)) + (__float128)tmp / (*nw->count);
            (*nw->count)++;
            shift--;
        }

        if (read_count < nw->max_memory)
            break;
    }
    return 0;
}