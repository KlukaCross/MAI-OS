#include <stdio.h>
#include "lib.h"

void print_help() {
    printf("USAGE:\n"
               "\t1 A B - the greatest common divisor of A and B\n"
               "\t2 K - calculation of Pi with accuracy K\n");
}

int main() {
    int arg;
    print_help();
    while(scanf("%d ", &arg) != EOF) {
        if (arg == 1) {
            int a, b;
            scanf("%d %d", &a, &b);
            printf("%d\n", gcd(a, b));
        } else if (arg == 2) {
            int k;
            scanf("%d", &k);
            printf("%f\n", Pi(k));
        } else {
            print_help();
            return -1;
        }
    }
    return 0;
}
