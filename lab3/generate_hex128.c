#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <quadmath.h>
#include "string_utils.h"
#include "constants.h"

int main(int argc, char* argv[]) {
    // prog filename number [min_value] [max_value]
    if (argc < 3 || argc > 5)
        return -1;
    char* filename = argv[1];
    long number = strtol(argv[2], NULL, 10);
    __float128 min = FLT128_MIN, max = FLT128_MAX;
    if (argc > 3)
        min = strtoflt128(argv[4], NULL);
    if (argc == 5)
        max = strtoflt128(argv[5], NULL);

    srand(time(NULL));
    FILE* f = fopen(filename, "w");
    for (long i = 0; i < number; ++i) {
        double a = rand()/(double)RAND_MAX;
        __float128 random_number = roundq((max-min)*a);
        char st[MAX_128_HEX_BYTES+1];
        float128_to_hex(random_number, (char **) &st);
        fwrite(st, sizeof(char), MAX_128_HEX_BYTES, f);
    }

    fclose(f);
    return 0;
}