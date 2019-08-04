#include <stdarg.h>
#include <stdio.h>

int func(char *str, ...);
int main() {
   func("Hoge: %s\n", "TEST", 12);
   return 0;
}
