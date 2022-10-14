#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "string_utils.h"
#include "constants.h"

int main(int argc, char* argv[]) {
    // prog filename number [min_value] [max_value]
    if (argc < 3 || argc > 5)
        return -1;
    char* filename = argv[1];
    long number = strtol(argv[2], NULL, 10);
    unsigned __int128 min = 0, max = -1;
    if (argc > 3)
        min = (unsigned __int128)strtold(argv[3], NULL);
    if (argc == 5)
        max = (unsigned __int128)strtold(argv[4], NULL);

    srand48(time(NULL));
    FILE* f = fopen(filename, "w");
    long double avg = 0;
    for (long i = 1; i <= number; ++i) {
        long double a = drand48();
        unsigned __int128 random_number = (__int128)(a*(max-min)) + min;
        char st[MAX_128_HEX_BYTES+1];
        int128_to_hex(random_number, st);
        fwrite(st, sizeof(char), MAX_128_HEX_BYTES, f);
        fwrite(" ", sizeof(char), 1, f);
        avg = avg * (1 - 1 / (long double)i) + (long double)random_number / i;
    }

    fclose(f);

    char st[1024];
    sprintf(st, "Number: %ld\nAverage: %Lf\n", number, avg);
    f = fopen(strcat(filename, "_info"), "w");
    fwrite(st, sizeof(char), strlen(st), f);
    fclose(f);
    return 0;
}