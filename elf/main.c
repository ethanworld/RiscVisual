// gcc -O0 main.c ./lib.so -g

#include <stdio.h>
#include "lib.h"

static int g_num = 123;
static char g_val[] = "abc";
int main(char args[])
{
    char val[] = "def";
    int a = 2, b = 3;
    int sum_val = sum(a, b);
    int mul_val = mul(sum_val, g_num);
    printf("result: %d, %d, %s %s\n", g_num, mul_val, g_val, val);
}