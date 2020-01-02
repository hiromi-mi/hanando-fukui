// preprocess.c
/*
Copyright 2020 hiromi-mi

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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// dirname()
#include <libgen.h>

extern Vector* tokens;

void preprocess(Vector *pre_tokens, char *fname) {
   Map *defined = new_map();
   // when compiled with hanando
   Token *hanando_fukui_compiled = malloc(sizeof(Token));
   hanando_fukui_compiled->ty = TK_NUM;
   hanando_fukui_compiled->pos = 0;
   hanando_fukui_compiled->num_val = 1;
   hanando_fukui_compiled->input = "__HANANDO_FUKUI__";
   map_put(defined, "__HANANDO_FUKUI__", hanando_fukui_compiled);

   int skipped = 0;
   int j, k;
   for (j = 0; j < pre_tokens->len; j++) {
      if (pre_tokens->data[j]->ty == '#') {
         // preprocessor begin
         j++;
         if (strcmp(pre_tokens->data[j]->input, "ifndef") == 0) {
            if (map_get(defined, pre_tokens->data[j + 2]->input)) {
               skipped = 1;
               // TODO skip until #endif
               // read because of defined.
            } else {
               while (pre_tokens->data[j]->ty != TK_NEWLINE &&
                      pre_tokens->data[j]->ty != TK_EOF) {
                  j++;
               }
            }
            continue;
         }
         if (strcmp(pre_tokens->data[j]->input, "ifdef") == 0) {
            if (!map_get(defined, pre_tokens->data[j + 2]->input)) {
               skipped = 1;
               // TODO skip until #endif
               // read because of defined.
            } else {
               while (pre_tokens->data[j]->ty != TK_NEWLINE &&
                      pre_tokens->data[j]->ty != TK_EOF) {
                  j++;
               }
            }
            continue;
         }
         if (strcmp(pre_tokens->data[j]->input, "endif") == 0) {
            skipped = 0;
            while (pre_tokens->data[j]->ty != TK_NEWLINE &&
                   pre_tokens->data[j]->ty != TK_EOF) {
               j++;
            }
            continue;
         }
         // skip without #ifdef, #endif. TODO dirty
         if (skipped != 0) {
            continue;
         }
         if (strcmp(pre_tokens->data[j]->input, "define") == 0) {
            map_put(defined, pre_tokens->data[j + 2]->input,
                    pre_tokens->data[j + 4]);
            while (pre_tokens->data[j]->ty != TK_NEWLINE &&
                   pre_tokens->data[j]->ty != TK_EOF) {
               j++;
            }
            continue;
         }
         if (strcmp(pre_tokens->data[j]->input, "include") == 0) {
            if (pre_tokens->data[j + 2]->ty == TK_STRING) {
               char *buf = NULL;
               if (fname) {
                  char *filename;
                  filename = strdup(fname);
                  char *basedirname = dirname(filename);
                  buf = malloc(sizeof(char) * 256);
                  snprintf(buf, 255, "%s/%s", basedirname,
                           pre_tokens->data[j + 2]->input);
               } else {
                  buf = pre_tokens->data[j + 2]->input;
               }
               preprocess(read_tokenize(buf), buf);
               // 最後のToken が EOF なので除く
               if (tokens->data[tokens->len-1]->ty == TK_EOF) {
                  tokens->len--;
               }
            }
            while (pre_tokens->data[j]->ty != TK_NEWLINE &&
                   pre_tokens->data[j]->ty != TK_EOF) {
               j++;
            }
         }
         continue;
      }
      // skip without #ifdef, #endif
      if (skipped != 0 || pre_tokens->data[j]->ty == TK_NEWLINE ||
          pre_tokens->data[j]->ty == TK_SPACE)
         continue;

      int called = 0;
      for (k = 0; k < defined->keys->len; k++) {
         char *chr = (char *)defined->keys->data[k];
         if (pre_tokens->data[j]->ty == TK_IDENT &&
             strcmp(pre_tokens->data[j]->input, chr) == 0) {
            called = 1;
            vec_push(tokens, defined->vals->data[k]);
            continue;
         }
      }
      if (!called) {
         vec_push(tokens, pre_tokens->data[j]);
      }
   }
}

