// main.c
/*


*/

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
   if (argc < 2) {
      puts("Incorrect Arguments");
      exit(1);
   }

   puts(".intel_syntax");
   puts(".global main");
   puts("main:");
   printf("mov rax, %ld\n", strtol(argv[1], NULL, 10));
   puts("ret");
   return 0;
}
