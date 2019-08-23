//#include <stdarg.h>
//#include <stdio.h>
#include "../../hoc_nyan/src/hoc.h"
int func(char *str, ...) {
   va_list ap;
   va_start(ap, str);
   /*char* arg = va_arg(ap, int);
   puts(arg);
   int arg2 = va_arg(ap, int);
   printf("%d\n", arg2);*/
   vprintf(str, ap);
   va_end(ap);
   return 0;
}

int main() {
   func("%d\n", 30);
   return 0;
}
