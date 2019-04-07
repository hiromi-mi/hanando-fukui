#include <stdio.h>
#include <stdlib.h>
int func(int arg) {
   printf("OK%d\n",arg);
   return 4;
}

int foo(int x, int y) { printf("%d\n", x + y);return 0;}

int* alloc2(int* p, int a, int b) {
   p = malloc(sizeof(int*) *2);
   printf("%d\n", p);
   *p = a;
   *(p+1) = b;
   return p;
}
