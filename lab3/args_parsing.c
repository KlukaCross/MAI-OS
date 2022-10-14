#include "args_parsing.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "constants.h"

void print_help(char name_program[]) {
    printf("SYNOPSIS:\n"
           "\t%s [OPTION] filename\n", name_program);
    printf("\t%s -h\n", name_program);
    printf("DESCRIPTION:\n"
           "\tПрограмма читает из файла filename 128 битные числа, записанные в "
           "шестнадцатиричном виде, "
           "и считает их среднее арифметическое c округлением результата до целых.\n\n");
    printf("\t-h\tпоказывает эту справку и выходит\n");
    printf("\t-t\tмаксимальное число потоков (по умолчанию %d)\n", DEFAULT_MAX_THREADS);
    printf("\t-m\tмаксимальное количество используемой оперативной памяти в байтах для "
           "считываемых чисел "
           "(по умолчанию %d). Внимание! Не рекомендуется устанавливать значение меньше "
           "%d*max_threads байт.\n",
           DEFAULT_MAX_MEMORY, MAX_128_HEX_BYTES+1);
}

int args_parsing(int argc, char* argv[], char** filename, long* max_threads, long* max_memory) {
    int arg_t = 0, arg_m = 0;

    if (argc == 2)
        *filename = argv[1];
    else if (argc == 4 || argc == 6) {
        if ((strcmp(argv[1], "-t") != 0 && strcmp(argv[1], "-m") != 0) ||
            (argc == 6 && strcmp(argv[3], "-t") != 0 && strcmp(argv[3], "-m") != 0)) {
            printf("Error: incorrect key\n");
            return -1;
        }

        if (!strcmp(argv[1], "-t")) arg_t = 2;
        else arg_m = 2;
        if (argc == 6) {
            if (!strcmp(argv[3], "-t")) arg_t = 4;
            else arg_m = 4;
            *filename = argv[5];
        } else
            *filename = argv[3];

        if (arg_t && (*max_threads = strtol(argv[arg_t], NULL, 10)) == 0) {
            printf("Error: incorrect value\n");
            return -1;
        }

        if (*max_threads <= 0) {
            printf("Error: max_threads must be greater than 0\n");
            return -1;
        }

        if (arg_m && (*max_memory = strtol(argv[arg_m], NULL, 10)) == 0) {
            printf("Error: incorrect value\n");
            return -1;
        }

        if (*max_memory <= 0) {
            printf("Error: max_memory must be greater than 0\n");
            return -1;
        }

        if (*max_memory < (MAX_128_HEX_BYTES+sizeof(__int128))*(*max_threads))
            printf("Warning: not recommended to set max_memory less than "
                   "%d*max_threads\n", MAX_128_HEX_BYTES+1);

    } else {
        printf("Error: incorrect number of arguments\n");
        return -1;
    }
    return 0;
}