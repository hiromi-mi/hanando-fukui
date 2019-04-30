#include "../main.h"
int main(int argc, char **argv) {
   if (argc < 2) {
      puts("Incorrect Arguments.\n");
      return 0;
   }
   puts(argv[1]);
   Token *token = malloc(sizeof(Token));
   token->ty = TK_EOF;
   token->input = "";
   int j;
   for (j = 0; j < 10; j++) {
      fprintf(stderr, "j: %d\n", j);
      j+1;
      fprintf(stderr, "j: %d\n", j);
      j+2;
      fprintf(stderr, "j: %d\n", j);
   }
   return 0;
}
