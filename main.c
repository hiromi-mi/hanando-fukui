// main.c
/*


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
   if (argc < 2) {
      puts("Incorrect Arguments");
      exit(1);
   }

   char *p = argv[1];

   puts(".intel_syntax");
   puts(".global main");
   puts("main:");
   printf("mov rax, %ld\n", strtol(p, &p, 10));
   while (*p != '\0') {
      switch (*p) {
         case '+':
            p++;
            printf("add rax, %ld\n", strtol(p, &p, 10));
            break;
         case '-':
            p++;
            printf("sub rax, %ld\n", strtol(p, &p, 10));
            break;
         default:
            puts("Error: Incorrect Char.");
            exit(1);
      }
   }
   puts("ret");
   return 0;
}
