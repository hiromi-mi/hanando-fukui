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
   double c = -3.6;
   printf("ND_NEG TEST\n%f\n", -c);
   printf("TEST\n%f\n", c);
   float d = -5.6;
   printf("ND_NEG TEST\n%f\n", (double)(-d));
   return 0;
}
