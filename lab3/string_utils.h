#ifndef LAB3_STRING_UTILS_H
#define LAB3_STRING_UTILS_H

#include <stdbool.h>
#include <quadmath.h>
#include "constants.h"

bool is_separator(char c);

__float128 hex_to_float128(char st[], int *index);
void float128_to_hex(__float128 number, char* st[MAX_128_HEX_BYTES+1]);

#endif //LAB3_STRING_UTILS_H
