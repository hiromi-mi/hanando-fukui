#include <stdio.h>
typedef struct {
   int a;
   char b;
   int c;
   void* d;
} Kind;


int func1(Kind* kind) {
   char i = kind->b;
   int j = kind->c;
   printf("%c, %d\n", i, j);
   return 8;
}
int func2(Kind* kind);
int main() {
   Kind e;
   e.a = 4;
   e.b = 'c';
   e.c = 17;
   //e.d = &e;
   func1(&e);
   func2(&e);
   return 0;
}

