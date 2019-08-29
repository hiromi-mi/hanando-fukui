#include <stdio.h>

auto func(int a) {
   int b;
   if (a <= 1) {
      return 1;
   } else {
      return a*func(a-1);
   }
}

int main() {
   printf("It must be 24: %d\n", func(4));
}
