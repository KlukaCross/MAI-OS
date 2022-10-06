#ifndef LAB3_ARGS_PARSING_H
#define LAB3_ARGS_PARSING_H

void print_help(char name_program[]);
int args_parsing(int argc, char* argv[], char** filename, long* max_threads, long* max_memory);

#endif
