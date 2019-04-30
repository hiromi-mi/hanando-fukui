#include "../main.h"
int main() {
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
