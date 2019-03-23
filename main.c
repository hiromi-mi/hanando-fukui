// main.c
/*


*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

// to use vector instead of something
Token tokens[100];

void tokenize(char *p) {
   int i = 0;
   while (*p != '\0') {
      if (isspace(*p)) {
         p++; continue;
      }
      if (*p == '+' || *p == '-') {
         tokens[i].ty = *p;
         tokens[i].token_str = p;
         i++;
         p++;
         continue;
      }
      if (isdigit(*p)) {
         tokens[i].ty = TK_NUM;
         tokens[i].token_str = p;
         tokens[i].num_val = strtol(p, &p, 10);
         i++;
         continue;
      }

      fprintf(stderr, "Cannot Tokenize: %s\n", p);
   }

   tokens[i].ty = TK_EOF;
   tokens[i].token_str = p;
}

int main(int argc, char **argv) {
   if (argc < 2) {
      puts("Incorrect Arguments");
      exit(1);
   }

   tokenize(argv[1]);
   //char *p = argv[1];

   puts(".intel_syntax");
   puts(".global main");
   puts("main:");
   if (tokens[0].ty != TK_NUM) {
      puts("Error: Incorrect Char.");
      exit(1);
   }
   printf("mov rax, %ld\n", tokens[0].num_val);

   int i=1;
   while (tokens[i].ty != TK_EOF) {
      switch (tokens[i].ty) {
         case '+':
            i++;
            if (tokens[i].ty != TK_NUM) {
               puts("Error: Incorrect Char.");
               exit(1);
            }
            printf("add rax, %ld\n", tokens[i].num_val);
            i++;
            break;
         case '-':
            i++;
            if (tokens[i].ty != TK_NUM) {
               puts("Error: Incorrect Char.");
               exit(1);
            }
            printf("sub rax, %ld\n", tokens[i].num_val);
            i++;
            break;
         default:
            puts("Error: Incorrect Char.");
            exit(1);
      }
   }
   puts("ret");
   return 0;
}
