#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "lib.h"

const char* LIBS[] = {"./liblib1d.so", "./liblib2d.so"};
typedef int (*gcd_func)(int A, int B);
typedef float (*Pi_func)(int k);

void print_help() {
    printf("USAGE:\n"
           "\t0 - change implementation\n"
           "\t1 A B - the greatest common divisor of A and B\n"
           "\t2 K - calculation of Pi with accuracy K\n");
}

void open_handle(void** lib_handle, const int cur_lib) {
    *lib_handle = dlopen(LIBS[cur_lib], RTLD_NOW);
    if (*lib_handle == NULL) {
        printf("%s\n", dlerror());
        exit(1);
    }
}

void set_implementation(gcd_func *gcdFunc, Pi_func *PiFunc, void* lib_handle) {
    *gcdFunc = dlsym(lib_handle, "gcd");
    if (*gcdFunc == NULL) {
        printf("%s\n", dlerror());
        exit(1);
    }
    *PiFunc = dlsym(lib_handle, "Pi");
    if (*PiFunc == NULL) {
        printf("%s\n", dlerror());
        exit(1);
    }
}

int main() {
    int arg, cur_lib = 0;
    void* lib_handle;
    gcd_func gcdFunc;
    Pi_func PiFunc;
    open_handle(&lib_handle, cur_lib);
    set_implementation(&gcdFunc, &PiFunc, lib_handle);
    print_help();
    while (scanf("%d", &arg) != EOF) {
        if (arg == 0) {
            dlclose(lib_handle);
            cur_lib = (cur_lib == 0)? 1 : 0;
            open_handle(&lib_handle, cur_lib);
            set_implementation(&gcdFunc, &PiFunc, lib_handle);
        } else if (arg == 1) {
            int a, b;
            scanf("%d %d", &a, &b);
            printf("%d\n", gcdFunc(a, b));
        } else if (arg == 2) {
            int k;
            scanf("%d", &k);
            printf("%f\n", PiFunc(k));
        } else {
            print_help();
            return -1;
        }
    }
    return 0;
}
