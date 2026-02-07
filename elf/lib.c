// gcc -O0 -fPIC -shared -Wall lib.c -o lib.so -g
# include "lib.h"
int sum (int a, int b) {
    return a + b;
}

int mul (int a, int b) {
    return a * b;
}