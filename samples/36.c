#include <stdio.h>
int main() {
   int a = -5;
   double b = 0.5;
   float c = 2.6f;
   printf("%d\n", a < a);
   printf("%f\n", a < b);
   printf("%f\n", a < c);
   printf("%f\n", b < c);
   printf("%d\n", a + a);
   printf("%f\n", a + b);
   printf("%f\n", (double)(a + c));
   printf("%f\n", b + c);
   printf("%d\n", a * a);
   printf("%f\n", a * b);
   printf("%f\n", (double)(a * c));
   printf("%f\n", b * c);
   return 0;
}
