// util.c
/*
Copyright 2019, 2020 hiromi-mi

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "main.h"
#include "selfhost.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern Vector *tokens;
extern int pos;
_Noreturn void error(const char *str, ...) {
   va_list ap;
   va_start(ap, str);
   if (tokens && tokens->len > pos) {
      char *buf = malloc(sizeof(char) * 256);
      vsnprintf(buf, 255, str, ap);
      fprintf(stderr, "%s on line %d pos %d: %s\n", buf,
              tokens->data[pos]->pline, pos, tokens->data[pos]->input);
   } else {
      vfprintf(stderr, str, ap);
   }
   va_end(ap);
   exit(1);
}

void test_map(void) {
   Vector *vec = new_vector();
   Token *hanando_fukui_compiled = malloc(sizeof(Token));
   hanando_fukui_compiled->ty = TK_NUM;
   hanando_fukui_compiled->pos = 0;
   hanando_fukui_compiled->num_val = 1;
   hanando_fukui_compiled->input = "HANANDO_FUKUI";
   vec_push(vec, hanando_fukui_compiled);
   vec_push(vec, (Token *)9);
   if (vec->len != 2) {
      error("Vector does not work yet!");
   }
   if (strcmp(vec->data[0]->input, "HANANDO_FUKUI") != 0) {
      error("Vector does not work yet!");
   }

   Map *map = new_map();
   map_put(map, "foo", hanando_fukui_compiled);
   if (map->keys->len != 1 || map->vals->len != 1) {
      error("Error: Map does not work yet!");
   }
   if ((long)map_get(map, "bar") != 0) {
      error("Error: Map does not work yet! on 3");
   }
   Token *te = map_get(map, "foo");
   if (strcmp(te->input, "HANANDO_FUKUI") != 0) {
      error("Error: Map does not work yet! on 3a");
   }
}

