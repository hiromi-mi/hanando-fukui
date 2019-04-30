#include <stdio.h>
#include <stdlib.h>
#include "../main.h"
Vector *new_vector() {
   Vector *vec = malloc(sizeof(Vector));
   vec->capacity = 16;
   vec->data = malloc(sizeof(Token *) * vec->capacity);
   vec->len = 0;
   return vec;
}

int vec_push(Vector *vec, Token *element) {
   if (vec->capacity == vec->len) {
      vec->capacity *= 2;
      vec->data = realloc(vec->data, sizeof(Token *) * vec->capacity);
   }
   vec->data[vec->len] = element;
   return vec->len++;
}

int main() {
   Vector *test = new_vector();
   Token *hanando_fukui_compiled = malloc(sizeof(Token));
   hanando_fukui_compiled->ty = TK_NUM;
   hanando_fukui_compiled->pos = 8;
   hanando_fukui_compiled->num_val = 1;
   hanando_fukui_compiled->input = "HANANDO_FUKUI";
   vec_push(test, hanando_fukui_compiled);
   fprintf(stderr,"%s %d\n",  test->data[0]->input, test->len);
   return 0;
}
