#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Map *new_map() {
   Map *map = malloc(sizeof(Map));
   map->keys = new_vector();
   map->vals = new_vector();
   return map;
}

void map_put(Map *map, char* key, void* val) {
   vec_push(map->keys, (void*)key);
   vec_push(map->vals, (void*)val);
}

void *map_get(Map *map, char* key) {
   for (int i=map->keys->len-1;i>=0;i--) {
      if (strcmp((char*)map->keys->data[i], key) == 0) {
         return map->vals->data[i];
      }
   }
   return NULL;
}

Vector *new_vector() {
   Vector* vec = malloc(sizeof(Vector));
   vec->capacity = 16;
   vec->data = malloc(sizeof(Token*) * vec->capacity);
   vec->len = 0;
   return vec;
}


void vec_push(Vector *vec, Token* element) {
   if (vec->capacity == vec->len) {
      vec->capacity *= 2;
      vec->data = realloc(vec->data, sizeof(Token*) * vec->capacity);
   }
   vec->data[vec->len++] = element;
}
