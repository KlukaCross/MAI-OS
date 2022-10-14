#include "string_utils.h"

unsigned __int128 hex_to_int(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return 10 + c - 'A';
    return 0;
}

char int_to_hex(unsigned __int128 c) {
    if (c >= 0 && c < 10)
        return '0' + c;
    else if (c > 9 && c < 16)
        return 'A' + c - 10;
    return '0';
}

unsigned __int128 hex_to_int128(char st[]) {
    unsigned __int128 number = 0;
    for (int i = 0; i < MAX_128_HEX_BYTES-1; ++i, st++) {
        unsigned __int128 hex = hex_to_int(*st);
        number = (number | hex) << 4;
    }
    unsigned __int128 hex = hex_to_int(*st);
    number |= hex;
    return number;
}

void int128_to_hex(unsigned __int128 number, char st[MAX_128_HEX_BYTES+1]) {
    for (int i = 1; i <= MAX_128_HEX_BYTES; ++i) {
        unsigned __int128 hex = number & (unsigned __int128)0b1111;
        number >>= 4;
        st[MAX_128_HEX_BYTES-i] = int_to_hex(hex);
    }
    st[MAX_128_HEX_BYTES+1] = '\0';
}