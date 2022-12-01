#include "lib.h"

// naive algorithm
int gcd(int A, int B) {
    int x = (A>B)? B : A;
    while (A % x != 0 || B % x != 0)
        x--;
    return x;
}

// Wallis formula
float Pi(int K) {
    float res = 1;
    for (int i = 1; i < K; i++){
        float num = 4*i*i;
        res *= num / (num-1);
    }
    return res * 2;
}
