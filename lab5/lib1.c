#include "lib.h"
#include "math.h"

// Euclid's algorithm
int gcd(int A, int B) {
    if (B == 0)
        return A;
    return gcd(B, A % B);
}

// Leibniz series
float Pi(int K) {
    float res = 0;
    float x = 1;
    for(int i = 0; i < K; i++){
        res += pow(-1, i) * (4 / x);
        x += 2;
    }
    return res;
}