#ifndef LAB3_STRING_UTILS_H
#define LAB3_STRING_UTILS_H

#include <stdbool.h>
#include "constants.h"

unsigned __int128 hex_to_int128(char st[]);
void int128_to_hex(unsigned __int128 number, char st[MAX_128_HEX_BYTES+1]);

#endif //LAB3_STRING_UTILS_H
