#include <stdarg.h>
#include <stdio.h>
int func(char *str, ...) {
   va_list ap;
   va_start(ap, str);
   char* arg = va_arg(ap, char*);
   puts(arg);
   int arg2 = va_arg(ap, int);
   printf("%d\n", arg2);
   va_end(ap);
   return 0;
}
