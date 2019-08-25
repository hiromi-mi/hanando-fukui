// float comparison
#include <stdio.h>

int main() {
   double a = 1.2;
   double b = 3.5;
   printf("%d ", a <b);
   printf("%d ", a <=b);
   printf("%d ", a >b);
   printf("%d\n", a >=b);
   a = 3.5;
   b = 1.2;
   printf("%d ", a <b);
   printf("%d ", a <=b);
   printf("%d ", a >b);
   printf("%d\n", a >=b);
   a = 3;
   b = 3;
   printf("%d ", a <b);
   printf("%d ", a <=b);
   printf("%d ", a >b);
   printf("%d\n", a >=b);
   return 0;
}
