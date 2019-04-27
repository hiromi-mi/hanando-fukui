#include <stdio.h>
typedef struct {
   int a;
   char b;
   int c;
   void* d;
   int e;
   char f;
   char g;
   int* h;
   int j;
   void* j;
} Kind;

int func2(Kind* kind) {
   char i = kind->b;
   int j = kind->c;
   printf("%c, %d\n", i, j);
   return 8;
}

