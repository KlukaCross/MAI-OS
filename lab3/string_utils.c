#include "string_utils.h"
#include <math.h>

bool is_separator(char c) {
    return c == ' ' || c == '\n';
}

int hex_to_int(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return 10 + c - 'A';
    return 0;
}

char int_to_hex(int c) {
    if (c >= 0 && c < 10)
        return '0' + c;
    else if (c > 9 && c < 16)
        return 'A' + c - 10;
    return '0';
}

__float128 hex_to_float128(char st[], int *index) {
    __float128 res = 0;
    int len_number = 0;
    while (*index > 0 && !is_separator(st[*index]) && st[*index] != '-') {
        res += (int)pow(16, len_number) * hex_to_int(st[*index]);
        (*index)--;
        len_number++;
    }
    if (st[*index] == '-') {
        (*index)--;
        return -res;
    }
    return res;
}

void float128_to_hex(__float128 number, char* st[MAX_128_HEX_BYTES+1]) {
    int c = 0;
    char buf[MAX_128_HEX_BYTES+1];
    while (number > 15) {
        __float128 ost = number - floorq(number/16)*16;
        number = floorq(number/16);
        buf[c++] = int_to_hex((int)ost);
    }
    for (int i = 1; i <= c; ++i) {
        *st[c-i] = buf[i-1];
    }
    *st[c] = '\0';
}
