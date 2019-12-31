// token.c
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

extern int lang;

Token *new_token(int pline, TokenConst ty, char *p) {
   Token *token = malloc(sizeof(Token));
   token->ty = ty;
   token->input = p;
   token->pline = pline;
   return token;
}

Vector *tokenize(char *p) {
   Vector *pre_tokens = new_vector();
   int pline = 1; // LINE No.
   while (*p != '\0') {
      if (*p == '\n') {
         Token *token = malloc(sizeof(Token));
         token->ty = TK_NEWLINE;
         vec_push(pre_tokens, token);
         while (*p == '\n' && *p != '\0') {
            pline++;
            p++;
         }
         continue;
      }
      if (*p == '/' && *(p + 1) == '/') {
         // skip because of one-lined comment
         while (*p != '\n' && *p != '\0')
            p++;
         continue;
      }
      if (*p == '/' && *(p + 1) == '*') {
         // skip because of one-lined comment
         while (*p != '\0') {
            if (*p == '*' && *(p + 1) == '/') {
               p += 2; // due to * and /
               /*
               if (*p == '\n') {
                  pline++;
               }*/
               break;
            }
            p++;
            if (*p == '\n') {
               pline++;
            }
         }
      }
      if (isspace(*p)) {
         vec_push(pre_tokens, new_token(pline, TK_SPACE, p));
         while (isspace(*p) && *p != '\0') {
            if (*p == '\n') {
               pline++;
            }
            p++;
         }
         continue;
      }

      if (*p == '\"') {
         Token *token = new_token(pline, TK_STRING, malloc(sizeof(char) * 256));
         int i = 0;
         while (*++p != '\"') {
            if (*p == '\\' && *(p + 1) == '\"') {
               // read 2 character and write them into 1 bytes
               token->input[i++] = *p;
               token->input[i++] = *(p + 1);
               p++;
            } else {
               token->input[i++] = *p;
            }
         }
         token->input[i] = '\0';
         vec_push(pre_tokens, token);
         p++; // skip "
         continue;
      }
      if (*p == '\'') {
         Token *token = new_token(pline, TK_NUM, p);
         token->type_size = 1; // to treat 'a' as char
         if (*(p + 1) != '\\') {
            token->num_val = *(p + 1);
         } else {
            switch (*(p + 2)) {
               case 'n':
                  token->num_val = '\n';
                  break;
               case 'b':
                  token->num_val = '\b';
                  break;
               case '0':
                  token->num_val = '\0';
                  break;
               case 't':
                  token->num_val = '\t';
                  break;
               case '\\':
                  token->num_val = '\\';
                  break;
               case '\"':
                  token->num_val = '\"';
                  break;
               case '\'':
                  token->num_val = '\'';
                  break;
               default:
                  error(
                      "Error: Error On this unsupported escape sequence: %d\n",
                      *(p + 2));
            }
            p++;
         }
         vec_push(pre_tokens, token);
         p += 3; // *p = '', *p+1 = a, *p+2 = '
         continue;
      }
      if ((*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '%' ||
           *p == '^' || *p == '|' || *p == '&') &&
          (*(p + 1) == '=')) {
         vec_push(pre_tokens, new_token(pline, '=', p + 1));
         vec_push(pre_tokens, new_token(pline, TK_OPAS, p));
         p += 2;
         continue;
      }

      if ((*p == '+' && *(p + 1) == '+') || (*p == '-' && *(p + 1) == '-')) {
         vec_push(pre_tokens, new_token(pline, *p + *(p + 1), p));
         p += 2;
         continue;
      }

      if ((*p == '|' && *(p + 1) == '|')) {
         vec_push(pre_tokens, new_token(pline, TK_OR, p));
         p += 2;
         continue;
      }
      if ((*p == '&' && *(p + 1) == '&')) {
         vec_push(pre_tokens, new_token(pline, TK_AND, p));
         p += 2;
         continue;
      }

      if ((*p == '-' && *(p + 1) == '>')) {
         vec_push(pre_tokens, new_token(pline, TK_ARROW, p));
         p += 2;
         continue;
      }

      if (strncmp(p, "...", 3) == 0) {
         vec_push(pre_tokens, new_token(pline, TK_OMIITED, p));
         p += 3;
         continue;
      }

      if ((*p == ':' && *(p + 1) == ':')) {
         vec_push(pre_tokens, new_token(pline, TK_COLONCOLON, p));
         p += 2;
         continue;
      }

      if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' ||
          *p == ')' || *p == ';' || *p == ',' || *p == '{' || *p == '}' ||
          *p == '%' || *p == '^' || *p == '|' || *p == '&' || *p == '?' ||
          *p == ':' || *p == '[' || *p == ']' || *p == '#' || *p == '.') {
         vec_push(pre_tokens, new_token(pline, *p, p));
         p++;
         continue;
      }
      if (*p == '=') {
         if (*(p + 1) == '=') {
            vec_push(pre_tokens, new_token(pline, TK_ISEQ, p));
            p += 2;
         } else {
            vec_push(pre_tokens, new_token(pline, '=', p));
            p++;
         }
         continue;
      }
      if (*p == '!') {
         if (*(p + 1) == '=') {
            vec_push(pre_tokens, new_token(pline, TK_ISNOTEQ, p));
            p += 2;
         } else {
            vec_push(pre_tokens, new_token(pline, *p, p));
            p++;
         }
         continue;
      }
      if (*p == '<') {
         if (*(p + 1) == '<') {
            if (*(p + 2) == '=') {
               vec_push(pre_tokens, new_token(pline, '=', p + 2));
               vec_push(pre_tokens, new_token(pline, TK_OPAS, p));
               p += 3;
            } else {
               vec_push(pre_tokens, new_token(pline, TK_LSHIFT, p));
               p += 2;
            }
         } else if (*(p + 1) == '=') {
            vec_push(pre_tokens, new_token(pline, TK_ISLESSEQ, p));
            p += 2;
         } else {
            vec_push(pre_tokens, new_token(pline, *p, p));
            p++;
         }
         continue;
      }
      if (*p == '>') {
         if (*(p + 1) == '>') {
            if (*(p + 2) == '=') {
               vec_push(pre_tokens, new_token(pline, '=', p + 2));
               vec_push(pre_tokens, new_token(pline, TK_OPAS, p));
               p += 3;
            } else {
               vec_push(pre_tokens, new_token(pline, TK_RSHIFT, p));
               p += 2;
            }
         } else if (*(p + 1) == '=') {
            vec_push(pre_tokens, new_token(pline, TK_ISMOREEQ, p));
            p += 2;
         } else {
            vec_push(pre_tokens, new_token(pline, *p, p));
            p++;
         }
         continue;
      }
      if (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
         Token *token = new_token(pline, TK_NUM, p);
         token->num_val = strtol(p + 2, &p, 16);
         token->type_size = 4; // to treat as int
         vec_push(pre_tokens, token);
         continue;
      }

      if (isdigit(*p)) {
         Token *token = new_token(pline, TK_NUM, p);
         token->num_val = strtol(p, &p, 10);
         token->type_size = 4; // to treat as int

         // if there are FLOAT
         if (*p == '.') {
            token->float_val = strtod(token->input, &p);
            if (*p == 'f') {
               // when ends with f:
               token->ty = TK_FLOAT;
               p++;
            } else {
               token->ty = TK_DOUBLE;
            }
         }
         vec_push(pre_tokens, token);
         continue;
      }

      if (('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z') || *p == '_') {
         Token *token = new_token(pline, TK_IDENT, malloc(sizeof(char) * 256));
         int j = 0;
         do {
            token->input[j] = *p;
            p++;
            j++;
            if (*p == ':' && *(p + 1) == ':') {
               token->input[j] = *p;
               p++;
               j++;
               token->input[j] = *p;
               p++;
               j++;
            }
         } while (('a' <= *p && *p <= 'z') || ('0' <= *p && *p <= '9') ||
                  ('A' <= *p && *p <= 'Z') || *p == '_');
         token->input[j] = '\0';

         if (strcmp(token->input, "if") == 0) {
            token->ty = TK_IF;
         } else if (strcmp(token->input, "else") == 0) {
            token->ty = TK_ELSE;
         } else if (strcmp(token->input, "do") == 0) {
            token->ty = TK_DO;
         } else if (strcmp(token->input, "while") == 0) {
            token->ty = TK_WHILE;
         } else if (strcmp(token->input, "for") == 0) {
            token->ty = TK_FOR;
         } else if (strcmp(token->input, "return") == 0) {
            token->ty = TK_RETURN;
         } else if (strcmp(token->input, "sizeof") == 0) {
            token->ty = TK_SIZEOF;
         } else if (strcmp(token->input, "goto") == 0) {
            token->ty = TK_GOTO;
         } else if (strcmp(token->input, "struct") == 0) {
            token->ty = TK_STRUCT;
         } else if (strcmp(token->input, "static") == 0) {
            token->ty = TK_STATIC;
         } else if (strcmp(token->input, "typedef") == 0) {
            token->ty = TK_TYPEDEF;
         } else if (strcmp(token->input, "break") == 0) {
            token->ty = TK_BREAK;
         } else if (strcmp(token->input, "continue") == 0) {
            token->ty = TK_CONTINUE;
         } else if (strcmp(token->input, "const") == 0) {
            token->ty = TK_CONST;
         } else if (strcmp(token->input, "NULL") == 0) {
            token->ty = TK_NULL;
         } else if (strcmp(token->input, "switch") == 0) {
            token->ty = TK_SWITCH;
         } else if (strcmp(token->input, "case") == 0) {
            token->ty = TK_CASE;
         } else if (strcmp(token->input, "default") == 0) {
            token->ty = TK_DEFAULT;
         } else if (strcmp(token->input, "enum") == 0) {
            token->ty = TK_ENUM;
         } else if (strcmp(token->input, "extern") == 0) {
            token->ty = TK_EXTERN;
         } else if (strcmp(token->input, "_Noreturn") == 0) {
            continue; // just skipping
         } else if (strcmp(token->input, "__LINE__") == 0) {
            token->ty = TK_NUM;
            token->num_val = pline;
            token->type_size = 8;
         }

         if (lang & 1) {
            if (strcmp(token->input, "class") == 0) {
               token->ty = TK_CLASS;
            } else if (strcmp(token->input, "public") == 0) {
               token->ty = TK_PUBLIC;
            } else if (strcmp(token->input, "private") == 0) {
               token->ty = TK_PRIVATE;
            } else if (strcmp(token->input, "protected") == 0) {
               token->ty = TK_PROTECTED;
            } else if (strcmp(token->input, "template") == 0) {
               token->ty = TK_TEMPLATE;
            } else if (strcmp(token->input, "typename") == 0) {
               token->ty = TK_TYPENAME;
            } else if (strcmp(token->input, "try") == 0) {
               token->ty = TK_TRY;
            } else if (strcmp(token->input, "catch") == 0) {
               token->ty = TK_CATCH;
            } else if (strcmp(token->input, "throw") == 0) {
               token->ty = TK_THROW;
            } else if (strcmp(token->input, "decltype") == 0) {
               token->ty = TK_DECLTYPE;
            } else if (strcmp(token->input, "new") == 0) {
               token->ty = TK_NEW;
            }
         }

         vec_push(pre_tokens, token);
         continue;
      }

      error("Cannot Tokenize: %s\n", p);
   }

   vec_push(pre_tokens, new_token(pline, TK_EOF, p));
   return pre_tokens;
}

Node *node_land(void) {
   Node *node = node_or();
   while (1) {
      if (consume_token(TK_AND)) {
         node = new_node(ND_LAND, node, node_or());
      } else {
         return node;
      }
   }
}

