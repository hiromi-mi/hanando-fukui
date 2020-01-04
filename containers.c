#include "main.h"
#include "selfhost.h"
#include <stdlib.h>
#include <string.h>

// Vector
Vector *new_vector(void) {
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
      if (!vec->data) {
         error("Error: Realloc failed: %ld\n", vec->capacity);
      }
   }
   vec->data[vec->len] = element;
   return vec->len++;
}

// Map
Map *new_map(void) {
   Map *map = malloc(sizeof(Map));
   map->keys = new_vector();
   map->vals = new_vector();
   return map;
}

int map_put(Map *map, const char *key, const void *val) {
   vec_push(map->keys, (void *)key);
   return vec_push(map->vals, (void *)val);
}

void *map_get(const Map *map, const char *key) {
   int i;
   for (i = map->keys->len - 1; i >= 0; i--) {
      if (strcmp((char *)map->keys->data[i], key) == 0) {
         return map->vals->data[i];
      }
   }
   return NULL;
}
