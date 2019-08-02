#include <stdarg.h>
#include <stdio.h>

int func(char *str, ...) {
   va_list ap;
   va_start(ap, str);
   vprintf(str, ap);
   va_end(ap);
   return 0;
}
int main() {
   func("Hoge: %s\n", "TEST");
   return 0;
}
