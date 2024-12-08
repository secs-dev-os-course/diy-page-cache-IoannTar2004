#include <stdio.h>

int x = 0;

void a(int d) {
    x = d > 0 ? d : x;
    printf("%d\n", x);
}