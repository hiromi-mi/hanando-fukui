// main.c
/*
Copyright 2019 hiromi-mi

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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEEK_END 2
#define SEEK_SET 0

#define NO_REGISTER NULL

// double ceil(double x);
// to use vector instead of something
Vector *tokens;
int pos = 0;
Node *code[300];
Map *global_vars;
Map *funcdefs;
Map *consts;
Vector *strs;
Map *typedb;
Map *struct_typedb;
Map *enum_typedb;

int env_for_while = 0;
int env_for_while_switch = 0;
Env *env;
int if_cnt = 0;
int for_while_cnt = 0;
char registers[6][4];

int type2size(Type *type);
Type *read_type(char **input);
Type *read_fundamental_type();
int confirm_type();
int confirm_ident();
int split_type_ident();
Vector *read_tokenize(char *fname);
void define_enum(int use);
char *expect_ident();
void program(Node *block_node);
Type *find_typed_db(char *input, Map *db);
int cnt_size(Type *type);
int get_lval_offset(Node *node);
Type *get_type_local(Node *node);
void gen(Node *node);

Register *gen_register_2(Node *node);
char *node2reg(Node *node, Register *reg);

Node *node_mul();
Node *node_term();
Node *node_mathexpr();
Node *node_mathexpr_without_comma();
Node *node_land();
Node *node_or();
Node *node_lor();
Node *node_and();
Node *node_xor();
Node *node_shift();
Node *node_add();
Node *node_cast();
Node *node_increment();
Node *assign();

// FOR SELFHOST
#ifdef __HANANDO_FUKUI__
FILE *fopen(char *name, char *type);
void *malloc(int size);
void *realloc(void *ptr, int size);
#endif

Map *new_map() {
   Map *map = malloc(sizeof(Map));
   map->keys = new_vector();
   map->vals = new_vector();
   return map;
}

int map_put(Map *map, char *key, void *val) {
   vec_push(map->keys, (void *)key);
   return vec_push(map->vals, (void *)val);
}

void *map_get(Map *map, char *key) {
   for (int i = map->keys->len - 1; i >= 0; i--) {
      if (strcmp(map->keys->data[i], key) == 0) {
         return map->vals->data[i];
      }
   }
   return NULL;
}

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
      if (!vec->data) {
         fprintf(stderr, "Error: Realloc failed: %ld\n", vec->capacity);
         exit(1);
      }
   }
   vec->data[vec->len] = element;
   return vec->len++;
}

void error(const char *str) {
   if (tokens) {
      fprintf(stderr, "%s on %d: %s\n", str, pos, tokens->data[pos]->input);
   } else {
      fprintf(stderr, "%s\n", str);
   }
   exit(1);
}

Node *new_string_node(char *_id) {
   Node *_node = malloc(sizeof(Node));
   _node->ty = ND_STRING;
   _node->lhs = NULL;
   _node->rhs = NULL;
   _node->name = _id;
   Type *type = malloc(sizeof(Type));
   type->ty = TY_PTR;
   type->ptrof = malloc(sizeof(Type));
   type->ptrof->ty = TY_CHAR;
   type->ptrof->ptrof = NULL;
   _node->type = type;
   return _node;
}

Node *new_node(NodeType ty, Node *lhs, Node *rhs) {
   Node *node = malloc(sizeof(Node));
   node->ty = ty;
   node->lhs = lhs;
   node->rhs = rhs;
   if (lhs) {
      node->type = node->lhs->type;
   }
   return node;
}

Node *new_char_node(long num_val) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_NUM;
   node->num_val = num_val;
   node->type = find_typed_db("char", typedb);
   return node;
}

Node *new_num_node(long num_val) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_NUM;
   node->num_val = num_val;
   node->type = find_typed_db("int", typedb);
   return node;
}
Node *new_long_num_node(long num_val) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_NUM;
   node->num_val = num_val;
   node->type = find_typed_db("long", typedb);
   return node;
}
Node *new_double_node(double num_val) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_FLOAT;
   node->num_val = num_val;
   node->type = find_typed_db("double", typedb);
   return node;
}

Node *new_deref_node(Node *lhs) {
   Node *node = new_node(ND_DEREF, lhs, NULL);
   node->type = node->lhs->type->ptrof;
   if (!node->type) {
      error("Error: Dereference on NOT pointered.");
   }
   return node;
}

Node *new_addsub_node(NodeType ty, Node *lhs_node, Node *rhs_node) {
   Node *lhs = lhs_node;
   Node *rhs = rhs_node;
   Node *node = NULL;
   if (lhs->type->ty == TY_PTR || lhs->type->ty == TY_ARRAY) {
      rhs = new_node('*', rhs, new_char_node(cnt_size(lhs->type->ptrof)));
   } else if (rhs->type->ty == TY_PTR || rhs->type->ty == TY_ARRAY) {
      lhs = new_node('*', lhs, new_char_node(cnt_size(rhs->type->ptrof)));
   }
   node = new_node(ty, lhs, rhs);
   if (rhs->type->ty == TY_PTR || rhs->type->ty == TY_ARRAY) {
      node->type = rhs->type;
      // TY_PTR no matter when lhs->lhs is INT or PTR
   }
   return node;
}

int cnt_size(Type *type) {
   switch (type->ty) {
      case TY_PTR:
      case TY_INT:
      case TY_CHAR:
      case TY_LONG:
         return type2size(type);
      case TY_ARRAY:
         return cnt_size(type->ptrof) * type->array_size;
      case TY_STRUCT:
         return type->offset;
      default:
         error("Error: on void type error.");
         return 0;
   }
}

Node *new_ident_node_with_new_variable(char *name, Type *type) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_IDENT;
   node->name = name;
   node->type = type;
   int size = cnt_size(type);
   // should aligned as x86_64
   if (size % 8 != 0) {
      size += (8 - size % 8);
   }
   env->rsp_offset += size;
   type->offset = env->rsp_offset;
   printf("#define: %s on %d\n", name, env->rsp_offset);
   map_put(env->idents, name, type);
   return node;
}

Node *new_ident_node(char *name) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_IDENT;
   node->name = name;
   if (strcmp(node->name, "stderr") == 0) {
      // TODO dirty
      node->ty = ND_EXTERN_SYMBOL;
      node->type = malloc(sizeof(Type));
      node->type->ty = TY_PTR;
      node->type->ptrof = malloc(sizeof(Type));
      node->type->ptrof->ty = TY_INT;
      return node;
   }
   node->type = get_type_local(node);
   if (node->type) {
      return node;
   }

   // Try global variable.
   node->ty = ND_GLOBAL_IDENT;
   node->type = map_get(global_vars, node->name);
   if (!node->type) {
      error("Error: New Variable Definition.");
   }
   return node;
}

Node *new_func_node(char *name) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_FUNC;
   node->name = name;
   node->lhs = NULL;
   node->rhs = NULL;
   node->type = NULL;
   node->argc = 0;
   Node *result = map_get(funcdefs, name);
   if (result) {
      node->type = result->type;
   } else {
      node->type = find_typed_db("int", typedb);
   }
   return node;
}

Env *new_env(Env *prev_env) {
   Env *_env = malloc(sizeof(Env));
   _env->env = prev_env;
   _env->idents = new_map();
   _env->rsp_offset = 0;
   if (prev_env) {
      _env->rsp_offset_all = prev_env->rsp_offset_all + prev_env->rsp_offset;
   } else {
      _env->rsp_offset_all = 0;
   }
   return _env;
}

Node *new_fdef_node(char *name, Env *prev_env, Type *type) {
   Node *node = malloc(sizeof(Node));
   node->type = type;
   node->ty = ND_FDEF;
   node->name = name;
   node->lhs = NULL;
   node->rhs = NULL;
   node->argc = 0;
   node->env = new_env(prev_env);
   node->code = new_vector();
   map_put(funcdefs, name, node);
   return node;
}

Node *new_block_node(Env *prev_env) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_BLOCK;
   node->lhs = NULL;
   node->rhs = NULL;
   node->argc = 0;
   node->type = NULL;
   node->env = new_env(prev_env);
   node->code = new_vector();
   return node;
}

int confirm_node(TokenConst ty) {
   if (tokens->data[pos]->ty != ty) {
      return 0;
   }
   return 1;
}

int consume_node(TokenConst ty) {
   if (tokens->data[pos]->ty != ty) {
      return 0;
   }
   pos++;
   return 1;
}

int expect_node(TokenConst ty) {
   if (tokens->data[pos]->ty != ty) {
      error("Error: Expected TokenConst are different.");
      return 0;
   }
   pos++;
   return 1;
}

Token *new_token(TokenConst ty, char *p) {
   Token *token = malloc(sizeof(Token));
   token->ty = ty;
   token->input = p;
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
      }
      if (*p == '/' && *(p + 1) == '*') {
         // skip because of one-lined comment
         while (*p != '\0') {
            if (*p == '*' && *(p + 1) == '/') {
               p += 2; // due to * and /
               break;
            }
            p++;
            if (*p == '\n') {
               pline++;
            }
         }
      }
      if (isspace(*p)) {
         vec_push(pre_tokens, new_token(TK_SPACE, p));
         while (isspace(*p) && *p != '\0') {
            p++;
         }
         continue;
      }

      if (*p == '\"') {
         Token *token = new_token(TK_STRING, malloc(sizeof(char) * 256));
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
         Token *token = new_token(TK_NUM, p);
         if (*(p + 1) != '\\') {
            token->num_val = *(p + 1);
         } else {
            switch (*(p + 2)) {
               case 'n':
                  token->num_val = '\n';
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
                  error("Error: Error On this escape sequence.");
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
         vec_push(pre_tokens, new_token('=', p + 1));
         vec_push(pre_tokens, new_token(TK_OPAS, p));
         p += 2;
         continue;
      }

      if ((*p == '+' && *(p + 1) == '+') || (*p == '-' && *(p + 1) == '-')) {
         vec_push(pre_tokens, new_token(*p + *(p + 1), p));
         p += 2;
         continue;
      }

      if ((*p == '|' && *(p + 1) == '|')) {
         vec_push(pre_tokens, new_token(TK_OR, p));
         p += 2;
         continue;
      }
      if ((*p == '&' && *(p + 1) == '&')) {
         vec_push(pre_tokens, new_token(TK_AND, p));
         p += 2;
         continue;
      }

      if ((*p == '-' && *(p + 1) == '>')) {
         vec_push(pre_tokens, new_token(TK_ARROW, p));
         p += 2;
         continue;
      }

      if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' ||
          *p == ')' || *p == ';' || *p == ',' || *p == '{' || *p == '}' ||
          *p == '%' || *p == '^' || *p == '|' || *p == '&' || *p == '?' ||
          *p == ':' || *p == '[' || *p == ']' || *p == '#' || *p == '.') {
         vec_push(pre_tokens, new_token(*p, p));
         p++;
         continue;
      }
      if (*p == '=') {
         if (*(p + 1) == '=') {
            vec_push(pre_tokens, new_token(TK_ISEQ, p));
            p += 2;
         } else {
            vec_push(pre_tokens, new_token('=', p));
            p++;
         }
         continue;
      }
      if (*p == '!') {
         if (*(p + 1) == '=') {
            vec_push(pre_tokens, new_token(TK_ISNOTEQ, p));
            p += 2;
         } else {
            vec_push(pre_tokens, new_token(*p, p));
            p++;
         }
         continue;
      }
      if (*p == '<') {
         if (*(p + 1) == '<') {
            vec_push(pre_tokens, new_token(TK_LSHIFT, p));
            p += 2;
         } else if (*(p + 1) == '=') {
            vec_push(pre_tokens, new_token(TK_ISLESSEQ, p));
            p += 2;
         } else {
            vec_push(pre_tokens, new_token(*p, p));
            p++;
         }
         continue;
      }
      if (*p == '>') {
         if (*(p + 1) == '>') {
            vec_push(pre_tokens, new_token(TK_RSHIFT, p));
            p += 2;
         } else if (*(p + 1) == '=') {
            vec_push(pre_tokens, new_token(TK_ISMOREEQ, p));
            p += 2;
         } else {
            vec_push(pre_tokens, new_token(*p, p));
            p++;
         }
         continue;
      }
      if (isdigit(*p)) {
         Token *token = new_token(TK_NUM, p);
         token->num_val = strtol(p, &p, 10);

         // if there are DOUBLE
         if (*p == '.') {
            token->ty = TK_FLOAT;
            token->num_val = strtod(token->input, &p);
         }
         vec_push(pre_tokens, token);
         continue;
      }

      if (('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z') || *p == '_') {
         Token *token = new_token(TK_IDENT, malloc(sizeof(char) * 256));
         int j = 0;
         do {
            token->input[j] = *p;
            p++;
            j++;
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
         } else if (strcmp(token->input, "__LINE__") == 0) {
            token->ty = TK_NUM;
            token->num_val = pline;
         }

         vec_push(pre_tokens, token);
         continue;
      }

      fprintf(stderr, "Cannot Tokenize: %s\n", p);
      exit(1);
   }

   vec_push(pre_tokens, new_token(TK_EOF, p));
   return pre_tokens;
}

Node *node_mathexpr_without_comma() { return node_lor(); }

Node *node_mathexpr() {
   Node *node = node_lor();
   while (1) {
      if (consume_node(',')) {
         node = new_node(',', node, node_lor());
         node->type = node->rhs->type;
      } else {
         return node;
      }
   }
}

Node *node_land() {
   Node *node = node_or();
   while (1) {
      if (consume_node(TK_AND)) {
         node = new_node(ND_LAND, node, node_or());
      } else {
         return node;
      }
   }
}

Node *node_lor() {
   Node *node = node_land();
   while (1) {
      if (consume_node(TK_OR)) {
         node = new_node(ND_LOR, node, node_land());
      } else {
         return node;
      }
   }
}

Node *node_shift() {
   Node *node = node_add();
   // TODO: Shift should be only char.
   while (1) {
      if (consume_node(TK_LSHIFT)) {
         node = new_node(ND_LSHIFT, node, node_add());
      } else if (consume_node(TK_RSHIFT)) {
         node = new_node(ND_RSHIFT, node, node_add());
      } else {
         return node;
      }
   }
}
Node *node_or() {
   Node *node = node_xor();
   while (1) {
      if (consume_node('|')) {
         node = new_node('|', node, node_xor());
      } else {
         return node;
      }
   }
}
Node *node_compare() {
   Node *node = node_shift();
   while (1) {
      if (consume_node('<')) {
         node = new_node('<', node, node_shift());
         node->type = find_typed_db("char", typedb);
      } else if (consume_node('>')) {
         node = new_node('>', node, node_shift());
         node->type = find_typed_db("char", typedb);
      } else if (consume_node(TK_ISLESSEQ)) {
         node = new_node(ND_ISLESSEQ, node, node_shift());
         node->type = find_typed_db("char", typedb);
      } else if (consume_node(TK_ISMOREEQ)) {
         node = new_node(ND_ISMOREEQ, node, node_shift());
         node->type = find_typed_db("char", typedb);
      } else {
         return node;
      }
   }
}

Node *node_iseq() {
   Node *node = node_compare();
   while (1) {
      if (consume_node(TK_ISEQ)) {
         node = new_node(ND_ISEQ, node, node_compare());
         node->type = find_typed_db("char", typedb);
      } else if (consume_node(TK_ISNOTEQ)) {
         node = new_node(ND_ISNOTEQ, node, node_compare());
         node->type = find_typed_db("char", typedb);
      } else {
         return node;
      }
   }
}

Node *node_and() {
   Node *node = node_iseq();
   while (1) {
      if (consume_node('&')) {
         node = new_node('&', node, node_iseq());
      } else {
         return node;
      }
   }
}
Node *node_xor() {
   Node *node = node_and();
   while (1) {
      if (consume_node('^')) {
         node = new_node('^', node, node_and());
      } else {
         return node;
      }
   }
}
Node *node_add() {
   Node *node = node_mul();
   while (1) {
      if (consume_node('+')) {
         node = new_addsub_node('+', node, node_mul());
      } else if (consume_node('-')) {
         node = new_addsub_node('-', node, node_mul());
      } else {
         return node;
      }
   }
}

Node *node_cast() {
   // reading cast stmt. This does not support multiple cast.
   Node *node = NULL;
   if (consume_node('(')) {
      if (confirm_type()) {
         Type *type = read_type(NULL);
         expect_node(')');
         node = new_node(ND_CAST, node_increment(), NULL);
         node->type = type;
         return node;
      }
      // because of consume_node'('
      // dirty
      pos--;
   }

   return node_increment();
}

Node *node_increment() {
   Node *node;
   if (consume_node(TK_PLUSPLUS)) {
      node = new_ident_node(expect_ident());
      node = new_node('=', node, new_addsub_node('+', node, new_num_node(1)));
   } else if (consume_node(TK_SUBSUB)) {
      node = new_ident_node(expect_ident());
      node = new_node('=', node, new_addsub_node('-', node, new_num_node(1)));
   } else if (consume_node('&')) {
      node = new_node(ND_ADDRESS, node_increment(), NULL);
      node->type = malloc(sizeof(Type));
      node->type->ty = TY_PTR;
      node->type->ptrof = node->lhs->type;
   } else if (consume_node('*')) {
      node = new_deref_node(node_increment());
   } else if (consume_node(TK_SIZEOF)) {
      if (consume_node('(') && confirm_type()) {
         // sizeof(int) read type without name
         Type *type = read_type(NULL);
         expect_node(')');
         return new_long_num_node(type2size(type));
      }
      node = node_mathexpr();
      return new_long_num_node(type2size(node->type));
   } else {
      return node_term();
   }
   return node;
}

Node *node_mul() {
   Node *node = node_cast();
   while (1) {
      if (consume_node('*')) {
         node = new_node('*', node, node_cast());
      } else if (consume_node('/')) {
         node = new_node('/', node, node_cast());
      } else if (consume_node('%')) {
         node = new_node('%', node, node_cast());
      } else {
         return node;
      }
   }
}

Node *new_dot_node(Node *node) {
   node = new_node('.', node, NULL);
   node->name = expect_ident();
   node->type = (Type *)map_get(node->lhs->type->structure, node->name);
   if (!node->type) {
      error("Error: structure not found.");
   }
   return node;
}

Node *read_complex_ident() {
   char *input = expect_ident();
   Node *node = NULL;
   for (int j = 0; j < consts->keys->len; j++) {
      // support for constant
      if (strcmp(consts->keys->data[j], input) == 0) {
         node = consts->vals->data[j];
         return node;
      }
   }
   node = new_ident_node(input);

   while (1) {
      if (consume_node('[')) {
         node = new_deref_node(new_addsub_node('+', node, node_mathexpr()));
         expect_node(']');
      } else if (consume_node('.')) {
         if (node->type->ty != TY_STRUCT) {
            error("Error: dot operator to NOT struct");
         }
         node = new_dot_node(node);
      } else if (consume_node(TK_ARROW)) {
         node = new_dot_node(new_deref_node(node));
      } else {
         return node;
      }
   }
}

Node *node_term() {
   if (consume_node('-')) {
      Node *node = new_node(ND_NEG, node_term(), NULL);
      return node;
   }
   if (consume_node('!')) {
      Node *node = new_node('!', node_term(), NULL);
      return node;
   }
   if (consume_node('+')) {
      Node *node = node_term();
      return node;
   }
   if (confirm_type(TK_FLOAT)) {
      Node *node = new_double_node((double)tokens->data[pos]->num_val);
      expect_node(TK_NUM);
      return node;
   }
   if (confirm_node(TK_NUM)) {
      Node *node = new_num_node(tokens->data[pos]->num_val);
      expect_node(TK_NUM);
      return node;
   }
   if (consume_node(TK_NULL)) {
      Node *node = new_num_node(0);
      node->type->ty = TY_PTR;
      return node;
   }
   if (confirm_ident()) {
      Node *node;
      // Function Call
      if (tokens->data[pos + 1]->ty == '(') {
         node = new_func_node(expect_ident());
         // skip func , (
         expect_node('(');
         while (1) {
            if ((consume_node(',') == 0) && consume_node(')')) {
               return node;
            }
            node->args[node->argc++] = node_mathexpr_without_comma();
         }
         // assert(node->argc <= 6);
         // pos++ because of consume_node(')')
         return node;
      }

      node = read_complex_ident();
      if (consume_node(TK_PLUSPLUS)) {
         node = new_node(ND_FPLUSPLUS, node, NULL);
         node->num_val = 1;
         if (node->lhs->type->ty == TY_PTR || node->lhs->type->ty == TY_ARRAY) {
            node->num_val = type2size(node->lhs->type->ptrof);
         }
      } else if (consume_node(TK_SUBSUB)) {
         node = new_node(ND_FSUBSUB, node, NULL);
         node->num_val = 1;
         if (node->lhs->type->ty == TY_PTR || node->lhs->type->ty == TY_ARRAY) {
            node->num_val = type2size(node->lhs->type->ptrof);
         }
      }
      // array
      return node;
   }
   if (confirm_node(TK_STRING)) {
      char *_str = malloc(sizeof(char) * 256);
      snprintf(_str, 255, ".LC%d", vec_push(strs, tokens->data[pos]->input));
      Node *node = new_string_node(_str);
      expect_node(TK_STRING);
      return node;
   }
   // Parensis
   if (consume_node('(')) {
      Node *node = assign();
      if (consume_node(')') == 0) {
         error("Error: Incorrect Parensis.");
      }
      return node;
   }
   fprintf(stderr, "Error: Incorrect Paresis without %c %d -> %c %d\n",
           tokens->data[pos - 1]->ty, tokens->data[pos - 1]->ty,
           tokens->data[pos]->ty, tokens->data[pos]->ty);
   exit(1);
}

Type *get_type_local(Node *node) {
   // TODO: another effect: set node->env
   Env *local_env = env;
   Type *type = NULL;
   while (type == NULL && local_env != NULL) {
      type = map_get(local_env->idents, node->name);
      if (type) {
         node->env = local_env;
         node->type = type;
         return type;
      }
      local_env = local_env->env;
   }
   node->type = type;
   return type;
}

// should be abolished
int get_lval_offset(Node *node) {
   Env *local_env = env;
   while (local_env != NULL) {
      node->type = map_get(local_env->idents, node->name);
      if (node->type) {
         return local_env->rsp_offset_all + node->type->offset;
      }
      local_env = local_env->env;
   }
   return 0;
}

char *_rax(Node *node) {
   if (node->type->ty == TY_CHAR) {
      return "al";
   } else if (node->type->ty == TY_INT) {
      return "eax";
   } else {
      return "rax";
   }
}

char *_rdi(Node *node) {
   if (node->type->ty == TY_CHAR) {
      return "dil";
   } else if (node->type->ty == TY_INT) {
      return "edi";
   } else {
      return "rdi";
   }
}

int cmp_regs(Node *node, Register *lhs_reg, Register *rhs_reg) {
   if (type2size(node->lhs->type) < type2size(node->rhs->type)) {
      // rdi, rax
      return printf("cmp %s, %s\n", node2reg(node->rhs, lhs_reg),
                    node2reg(node->rhs, rhs_reg));
   } else {
      return printf("cmp %s, %s\n", node2reg(node->lhs, lhs_reg),
                    node2reg(node->lhs, rhs_reg));
   }
}

int cmp_rax_rdi(Node *node) {
   if (type2size(node->lhs->type) < type2size(node->rhs->type)) {
      // rdi, rax
      return printf("cmp %s, %s\n", _rax(node->rhs), _rdi(node->rhs));
   } else {
      return printf("cmp %s, %s\n", _rax(node->lhs), _rdi(node->lhs));
   }
}

void gen_lval(Node *node) {
   if (node->ty == ND_EXTERN_SYMBOL) {
      printf("mov rax, qword ptr [rip + %s@GOTPCREL]\n", node->name);
      puts("push rax");
      return;
   }
   if (node->ty == ND_IDENT) {
      int offset = get_lval_offset(node);
      puts("mov rax, rbp");
      printf("sub rax, %d\n", offset);
      puts("push rax");
      return;
   }

   if (node->ty == ND_GLOBAL_IDENT) {
      printf("xor rax, rax\n");
      printf("lea rax, qword ptr %s[rip]\n", node->name);
      puts("push rax");
      return;
   }

   if (node->ty == ND_DEREF) {
      gen(node->lhs);
      return;
   }
   if (node->ty == '.') {
      gen_lval(node->lhs);
      puts("pop rax");
      printf("add rax, %d\n", node->type->offset);
      puts("push rax");
      return;
   }

   error("Error: Incorrect Variable of lvalue");
}

int type2size(Type *type) {
   if (!type) {
      return 0;
   }
   switch (type->ty) {
      case TY_PTR:
         return 8;
      case TY_LONG:
         return 8;
      case TY_INT:
         return 4;
      case TY_CHAR:
         return 1;
      case TY_ARRAY:
         return cnt_size(type->ptrof);
      case TY_STRUCT:
         return cnt_size(type);
      default:
         error("Error: NOT a type");
         return 0;
   }
}

int reg_table[6];
char *registers8[6];
char *registers16[6];
char *registers32[6];
char *registers64[6];

// These registers will be used to map into registers
void init_reg_registers() {
   // This code is valid (and safe) because RHS is const ptr. lreg[7] -> on top
   // of "r10b"
   registers8[0] = "r10b";
   registers8[1] = "r11b";
   registers8[2] = "r12b";
   registers8[3] = "r13b";
   registers8[4] = "r14b";
   registers8[5] = "r15b";
   registers16[0] = "r10w";
   registers16[1] = "r11w";
   registers16[2] = "r12w";
   registers16[3] = "r13w";
   registers16[4] = "r14w";
   registers16[5] = "r15w";
   registers32[0] = "r10d";
   registers32[1] = "r11d";
   registers32[2] = "r12d";
   registers32[3] = "r13d";
   registers32[4] = "r14d";
   registers32[5] = "r15d";
   registers64[0] = "r10";
   registers64[1] = "r11";
   registers64[2] = "r12";
   registers64[3] = "r13";
   registers64[4] = "r14";
   registers64[5] = "r15";
}

char *id2reg8(int id) { return registers8[id]; }

char *id2reg32(int id) { return registers32[id]; }

char *id2reg64(int id) { return registers64[id]; }

void init_reg_table() {
   for (int j = 0; j < 6; j++) {
      reg_table[j] = 0;
   }
}

char *gvar_node2reg(Node *node, char* name) {
   char *_str = malloc(sizeof(char) * 256);
   if (node->type->ty == TY_CHAR) {
      snprintf(_str, 255, "byte ptr %s[rip]", name);
   } else if (node->type->ty == TY_INT) {
      snprintf(_str, 255, "dword ptr %s[rip]", name);
   } else {
      snprintf(_str, 255, "qword ptr %s[rip]", name);
   }
   return _str;
}

char *node2reg(Node *node, Register *reg) {
   if (reg->kind == R_REGISTER) {
      if (node->type->ty == TY_CHAR) {
         return id2reg8(reg->id);
      } else if (node->type->ty == TY_INT) {
         return id2reg32(reg->id);
      } else {
         return id2reg64(reg->id);
      }
      // with type specifier.
   } else if (reg->kind == R_LVAR) {
      char *_str = malloc(sizeof(char) * 256);
      if (node->type->ty == TY_CHAR) {
         snprintf(_str, 255, "byte ptr [rbp-%d]", reg->id);
      } else if (node->type->ty == TY_INT) {
         snprintf(_str, 255, "dword ptr [rbp-%d]", reg->id);
      } else {
         snprintf(_str, 255, "qword ptr [rbp-%d]", reg->id);
      }
      return _str;
   } else if (reg->kind == R_GVAR) {
      return gvar_node2reg(node, reg->name);
   }
   fprintf(stderr, "Error: Cannot Have Register\n");
   exit(1);
}

Register *use_temp_reg() {
   for (int j = 0; j < 6; j++) {
      if (reg_table[j] > 0)
         continue;
      reg_table[j] = 1;

      Register *reg = malloc(sizeof(Register));
      reg->id = j;
      reg->name = NULL;
      reg->kind = R_REGISTER;
      return reg;
   }
   fprintf(stderr, "No more registers are avaliable\n");
   return 0;
}

void finish_reg(Register *reg) {
   if (reg->kind == R_REGISTER) {
      reg_table[reg->id] = 0;
   }
   free(reg);
}

void secure_mutable(Register *reg) {
   // to enable to change
   if (reg->kind != R_REGISTER) {
      Register *new_reg = use_temp_reg();
      reg->id = new_reg->id;
      reg->kind = new_reg->kind;
      reg->name = new_reg->name;
   }
}

Register *gen_register_3(Node *node) {
   Register *temp_reg;
   // Treat as lvalue.
   if (!node) {
      return NO_REGISTER;
   }
   switch (node->ty) {
      case ND_IDENT:
         temp_reg = use_temp_reg();
         printf("lea %s, [rbp-%d]\n", node2reg(node, temp_reg),
                get_lval_offset(node));
         return temp_reg;

      case ND_DEREF:
         return gen_register_2(node->lhs);

      case ND_GLOBAL_IDENT:
         temp_reg = use_temp_reg();
         printf("lea %s, %s\n", node2reg(node, temp_reg), gvar_node2reg(node, node->name));
         return temp_reg;

      default:
         error("Error: NOT lvalue");
   }
}

void extend_al_ifneeded(Node *node, Register *reg) {
   if (type2size(node->type) > 1) {
      printf("movzx %s, al\n", node2reg(node, reg));
   } else {
      printf("mov %s, al\n", node2reg(node, reg));
   }
}

Register *gen_register_2(Node *node) {
   Register *temp_reg, *lhs_reg, *rhs_reg;
   int j = 0;
   Env *prev_env;

   if (!node) {
      return NO_REGISTER;
   }
   switch (node->ty) {
      case ND_NUM:
         temp_reg = use_temp_reg();
         switch (node->type->ty) {
            case TY_CHAR:
               printf("movxz %s, %ld\n", node2reg(node, temp_reg),
                      node->num_val);
               break;
            default:
               printf("mov %s, %ld\n", node2reg(node, temp_reg), node->num_val);
         }
         return temp_reg;

      case ND_STRING:
         temp_reg = malloc(sizeof(Register));
         temp_reg->id = get_lval_offset(node);
         temp_reg->kind = R_GVAR;
         return temp_reg;
         // return with toplevel char ptr.

      case ND_IDENT:
         temp_reg = malloc(sizeof(Register));
         temp_reg->id = get_lval_offset(node);
         temp_reg->kind = R_LVAR;
         temp_reg->name = NULL;
         return temp_reg;

      case ND_GLOBAL_IDENT:
         temp_reg = malloc(sizeof(Register));
         temp_reg->id = 0;
         temp_reg->kind = R_GVAR;
         temp_reg->name = node->name;
         return temp_reg;

      case ND_EQUAL:
         // This behaivour can be revised. like [rbp-8+2]
         if (node->lhs->ty == ND_IDENT || node->lhs->ty == ND_GLOBAL_IDENT) {
            lhs_reg = gen_register_2(node->lhs);
            rhs_reg = gen_register_2(node->rhs);
            printf("mov %s, %s\n", node2reg(node->lhs, lhs_reg),
                   node2reg(node->rhs, rhs_reg));
            finish_reg(rhs_reg);
            return lhs_reg;
         } else {
            lhs_reg = gen_register_3(node->lhs);
            rhs_reg = gen_register_2(node->rhs);
            // This Should be adjusted with ND_CAST
            printf("mov [%s], %s\n", id2reg64(lhs_reg->id),
                   node2reg(node->rhs, rhs_reg));
            finish_reg(lhs_reg);
            return rhs_reg;
         }

      case ND_CAST:
         temp_reg = gen_register_2(node->lhs);
         switch (node->lhs->type->ty) {
            case TY_CHAR:
               // TODO treat as unsigned char.
               // for signed char, use movsx instead of.
               printf("movzx %s, %s\n", node2reg(node, temp_reg),
                      id2reg8(temp_reg->id));
               break;
            default:
               break;
         }
         return temp_reg;

      case ND_ADD:
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         secure_mutable(lhs_reg);
         printf("add %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         finish_reg(rhs_reg);
         return lhs_reg;

      case ND_SUB:
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         secure_mutable(lhs_reg);
         // TODO Fix for size convergence.
         printf("sub %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         finish_reg(rhs_reg);
         return lhs_reg;

      case ND_MUL:
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         secure_mutable(lhs_reg);
         printf("imul %s, %s\n", node2reg(node->lhs, lhs_reg),
                node2reg(node->rhs, rhs_reg));
         finish_reg(rhs_reg);
         return lhs_reg;

      case ND_DIV:
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         // TODO should support char
         printf("mov %s, %s\n", _rax(node->lhs), node2reg(node->lhs, lhs_reg));
         puts("cqo");
         printf("idiv %s\n", node2reg(node->rhs, rhs_reg));
         secure_mutable(lhs_reg);
         printf("mov %s, %s\n", node2reg(node->lhs, lhs_reg), _rax(node->lhs));
         finish_reg(rhs_reg);
         return lhs_reg;

      case ND_MOD:
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         // TODO should support char: idiv only support r32 or r64
         printf("mov %s, %s\n", _rax(node->lhs), node2reg(node->lhs, lhs_reg));
         puts("cqo");
         printf("idiv %s\n", node2reg(node->rhs, rhs_reg));
         secure_mutable(lhs_reg);
         printf("mov %s, %s\n", node2reg(node->lhs, lhs_reg), "rdx");
         finish_reg(rhs_reg);
         return lhs_reg;

      case ND_XOR:
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         secure_mutable(lhs_reg);
         printf("xor %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         finish_reg(rhs_reg);
         return lhs_reg;

      case ND_AND:
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         secure_mutable(lhs_reg);
         printf("and %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         finish_reg(rhs_reg);
         return lhs_reg;
         break;

      case ND_OR:
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         secure_mutable(lhs_reg);
         printf("or %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         finish_reg(rhs_reg);
         return lhs_reg;

      case ND_RSHIFT:
         // SHOULD be int
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         // FIXME: for signed int (Arthmetric)
         // mov rdi[8] -> rax
         printf("mov cl, %s\n", node2reg(node->rhs, rhs_reg));
         finish_reg(rhs_reg);
         secure_mutable(lhs_reg);
         printf("sar %s, cl\n", node2reg(node->lhs, lhs_reg));
         return lhs_reg;

      case ND_LSHIFT:
         // SHOULD be int
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         // FIXME: for signed int (Arthmetric)
         // mov rdi[8] -> rax
         printf("mov cl, %s\n", node2reg(node->rhs, rhs_reg));
         finish_reg(rhs_reg);
         secure_mutable(lhs_reg);
         printf("sal %s, cl\n", node2reg(node->lhs, lhs_reg));
         return lhs_reg;

      case ND_ISEQ:
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         cmp_regs(node, lhs_reg, rhs_reg);
         puts("sete al");
         finish_reg(lhs_reg);
         finish_reg(rhs_reg);

         temp_reg = use_temp_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_ISNOTEQ:
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         cmp_regs(node, lhs_reg, rhs_reg);
         puts("setne al");
         finish_reg(lhs_reg);
         finish_reg(rhs_reg);

         temp_reg = use_temp_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_GREATER:
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         cmp_regs(node, lhs_reg, rhs_reg);
         puts("setg al");
         finish_reg(lhs_reg);
         finish_reg(rhs_reg);

         temp_reg = use_temp_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_LESS:
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         cmp_regs(node, lhs_reg, rhs_reg);
         // TODO: is "andb 1 %al" required?
         puts("setl al");
         finish_reg(lhs_reg);
         finish_reg(rhs_reg);

         temp_reg = use_temp_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_ISMOREEQ:
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         cmp_regs(node, lhs_reg, rhs_reg);
         puts("setge al");
         puts("and al, 1");
         finish_reg(lhs_reg);
         finish_reg(rhs_reg);

         temp_reg = use_temp_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_ISLESSEQ:
         lhs_reg = gen_register_2(node->lhs);
         rhs_reg = gen_register_2(node->rhs);
         cmp_regs(node, lhs_reg, rhs_reg);
         puts("setle al");
         puts("and al, 1");
         finish_reg(lhs_reg);
         finish_reg(rhs_reg);

         temp_reg = use_temp_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_RETURN:
         if (node->lhs) {
            lhs_reg = gen_register_2(node->lhs);
            printf("mov %s, %s\n", _rax(node->lhs),
                   node2reg(node->lhs, lhs_reg));
            finish_reg(lhs_reg);
         }
         puts("mov rsp, rbp");
         puts("pop rbp");
         puts("ret");
         return NO_REGISTER;

      case ND_ADDRESS:
         temp_reg = use_temp_reg();
         lhs_reg = gen_register_2(node->lhs);
         printf("lea %s, %s\n", node2reg(node, temp_reg),
                node2reg(node, lhs_reg));
         finish_reg(lhs_reg);
         return temp_reg;

      case ND_NEG:
         lhs_reg = gen_register_2(node->lhs);
         printf("neg %s\n", node2reg(node, lhs_reg));
         return lhs_reg;

      case ND_FDEF:
         prev_env = env;
         env = node->env;

         printf(".type %s,@function\n", node->name);
         printf(".global %s\n", node->name); // to visible for ld
         printf("%s:\n", node->name);
         puts("push rbp");
         puts("mov rbp, rsp");
         printf("sub rsp, %d\n", env->rsp_offset);
         for (j = 0; node->code->data[j]; j++) {
            // read inside functions.
            gen_register_2(node->code->data[j]);
         }
         puts("mov rsp, rbp");
         puts("pop rbp");
         puts("ret");

         env = prev_env;
         return NO_REGISTER;

      case ND_FUNC:
         for (j = 0; j < node->argc; j++) {
            gen_register_2(node->args[j]);
         }
         if (node->type->ty == TY_VOID) {
            return NO_REGISTER;
         } else {
            temp_reg = use_temp_reg();
            printf("mov %s, %s\n", node2reg(node, temp_reg), _rax(node));
            return temp_reg;
         }

      case ND_GOTO:
         printf("jmp %s\n", node->name);
         return NO_REGISTER;

      default:
         fprintf(stderr, "Error: Incorrect Registers.\n");
         exit(1);
   }
   return NO_REGISTER;
}

void gen_register(Node *node) {
   if (!node) {
      return;
   }

   // TODO: Support Global Variable
   gen_register_2(node);
}

void gen_register_top() {
   init_reg_table();
   init_reg_registers();
   // consume code[j] and get
   gen_register(code[0]);
}

void gen(Node *node) {
   if (!node) {
      return;
   }
   if (node->ty == ND_NUM) {
      printf("push %ld\n", node->num_val);
      return;
   }
   if (node->ty == ND_FLOAT) {
      printf("push %ld\n", node->num_val);
      return;
   }
   if (node->ty == ND_STRING) {
      printf("lea rax, qword ptr %s[rip]\n", node->name);
      puts("push rax");
      return;
   }

   if (node->ty == ND_SWITCH) {
      int cur_if_cnt = for_while_cnt++;
      int prev_env_for_while_switch = env_for_while_switch;
      env_for_while_switch = cur_if_cnt;
      gen(node->lhs);
      puts("pop rbx");
      // find CASE Labels and lookup into args[0]->code->data
      for (int j = 0; node->rhs->code->data[j]; j++) {
         Node *curnode = (Node *)node->rhs->code->data[j];
         if (curnode->ty == ND_CASE) {
            char *input = malloc(sizeof(char) * 256);
            snprintf(input, 255, ".L%dC%d", cur_if_cnt, j);
            curnode->name = input; // assign unique ID
            gen(curnode->lhs);
            puts("pop rax");
            printf("cmp rbx, rax\n");
            printf("je %s\n", input);
         }
         if (curnode->ty == ND_DEFAULT) {
            char *input = malloc(sizeof(char) * 256);
            snprintf(input, 255, ".L%dC%d", cur_if_cnt, j);
            curnode->name = input;
            printf("jmp %s\n", input);
         }
      }
      // content
      gen(node->rhs);
      printf(".Lend%d:\n", cur_if_cnt);
      env_for_while_switch = prev_env_for_while_switch;
      return;
   }

   if (node->ty == ND_CASE || node->ty == ND_DEFAULT) {
      // just an def. of goto
      // saved with input
      if (!node->name) {
         error("Error: case statement without switch\n");
      }
      printf("%s:\n", node->name);
      return;
   }

   if (node->ty == ND_BLOCK) {
      Env *prev_env = env;
      env = node->env;
      for (int j = 0; node->code->data[j]; j++) {
         gen(node->code->data[j]);
      }
      env = prev_env;
      return;
   }

   if (node->ty == ND_FDEF) {
      Env *prev_env = env;
      env = node->env;
      printf(".type %s,@function\n", node->name);
      // to visible for ld
      printf(".global %s\n", node->name);
      printf("%s:\n", node->name);
      puts("push rbp");
      puts("mov rbp, rsp");
      printf("sub rsp, %d\n", env->rsp_offset);
      // char registers[6][4] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
      for (int j = 0; j < node->argc; j++) {
         gen_lval(node->args[j]);
         puts("pop rax");
         printf("mov [rax], %s\n", registers[j]);
         puts("push rax");
      }
      for (int j = 0; node->code->data[j]; j++) {
         if (node->code->data[j]->ty == ND_RETURN) {
            Node *curnode = node->code->data[j];
            gen(curnode->lhs);
            puts("pop rax");
            break;
         }
         // read inside functions.
         gen(node->code->data[j]);
         puts("pop rax");
      }
      puts("mov rsp, rbp");
      puts("pop rbp");
      puts("ret");
      env = prev_env;
      return;
   }

   if (node->ty == ND_LOR) {
      int cur_if_cnt = if_cnt++;
      gen(node->lhs);
      printf("pop rdi\n");
      printf("cmp %s, 0\n", _rdi(node->lhs));
      printf("jne .Lorend%d\n", cur_if_cnt);
      gen(node->rhs);
      puts("pop rdi");
      printf("cmp %s, 0\n", _rdi(node->rhs));
      puts("setne al");
      puts("movzx rax, al");
      printf(".Lorend%d:\n", cur_if_cnt);
      puts("setne al");
      puts("movzx rax, al");
      puts("push rax");
      return;
   }
   if (node->ty == ND_LAND) {
      int cur_if_cnt = if_cnt++;
      gen(node->lhs);
      puts("pop rdi");
      printf("cmp %s, 0\n", _rdi(node->lhs));
      printf("je .Lorend%d\n", cur_if_cnt);
      gen(node->rhs);
      puts("pop rdi");
      printf("cmp %s, 0\n", _rdi(node->rhs));
      printf(".Lorend%d:\n", cur_if_cnt);
      puts("setne al");
      puts("movzx rax, al");
      puts("push rax");
      return;
   }

   if (node->ty == ND_BREAK) {
      printf("jmp .Lend%d #break\n", env_for_while_switch);
      return;
   }
   if (node->ty == ND_CONTINUE) {
      printf("jmp .Lbegin%d\n", env_for_while);
      return;
   }
   if (node->ty == ND_CAST) {
      gen(node->lhs);
      if (node->type->ty == TY_INT && type2size(node->type) == 8) {
         puts("cdqe");
      }
      return;
   }

   if (node->ty == ND_FUNC) {
      for (int j = 0; j < node->argc; j++) {
         gen(node->args[j]);
      }
      for (int j = node->argc - 1; j >= 0; j--) {
         // because of function call will break these registers
         printf("pop %s\n", registers[j]);
      }
      // printf("sub rsp, %d\n", (int)(ceil(4 * node->argc / 16.)) * 16);
      // FIXME: alignment should be 64-bit
      puts("mov al, 0");               // TODO to preserve float
      printf("call %s\n", node->name); // rax should be aligned with the size
      if (node->type->ty == TY_VOID) {
         puts("push 0"); // FIXME
      } else {
         puts("push rax");
      }
      return;
   }

   if (node->ty == ND_IF) {
      gen(node->args[0]);
      puts("pop rax");
      printf("cmp %s, 0\n", _rax(node->args[0]));
      int cur_if_cnt = if_cnt++;
      printf("je .Lendif%d\n", cur_if_cnt);
      gen(node->lhs);
      if (node->rhs) {
         printf("jmp .Lelseend%d\n", cur_if_cnt);
      }
      printf(".Lendif%d:\n", cur_if_cnt);
      if (node->rhs) {
         // There are else
         gen(node->rhs);
         printf(".Lelseend%d:\n", cur_if_cnt);
      }
      return;
   }

   if (node->ty == ND_WHILE) {
      int cur_if_cnt = for_while_cnt++;
      int prev_env_for_while = env_for_while;
      int prev_env_for_while_switch = env_for_while_switch;
      env_for_while = cur_if_cnt;
      env_for_while_switch = cur_if_cnt;

      printf(".Lbegin%d:\n", cur_if_cnt);
      gen(node->lhs);
      puts("pop rax");
      printf("cmp %s, 0\n", _rax(node->lhs));
      printf("je .Lend%d\n", cur_if_cnt);
      gen(node->rhs);
      printf("jmp .Lbegin%d\n", cur_if_cnt);
      printf(".Lend%d:\n", cur_if_cnt);

      env_for_while = prev_env_for_while;
      env_for_while_switch = prev_env_for_while_switch;
      return;
   }
   if (node->ty == ND_DOWHILE) {
      int cur_if_cnt = for_while_cnt++;
      int prev_env_for_while = env_for_while;
      int prev_env_for_while_switch = env_for_while_switch;
      env_for_while = cur_if_cnt;
      env_for_while_switch = cur_if_cnt;
      printf(".Lbegin%d:\n", cur_if_cnt);
      gen(node->rhs);
      gen(node->lhs);
      puts("pop rax");
      printf("cmp %s, 0\n", _rax(node->lhs));
      printf("jne .Lbegin%d\n", cur_if_cnt);
      printf(".Lend%d:\n", cur_if_cnt);
      env_for_while = prev_env_for_while;
      env_for_while_switch = prev_env_for_while_switch;
      return;
   }

   if (node->ty == ND_FOR) {
      int cur_if_cnt = for_while_cnt++;
      int prev_env_for_while = env_for_while;
      int prev_env_for_while_switch = env_for_while_switch;
      env_for_while = cur_if_cnt;
      env_for_while_switch = cur_if_cnt;
      gen(node->args[0]);
      printf("jmp .Lcondition%d\n", cur_if_cnt);
      printf(".Lbeginwork%d:\n", cur_if_cnt);
      gen(node->rhs);
      printf(".Lbegin%d:\n", cur_if_cnt);
      gen(node->args[2]);
      printf(".Lcondition%d:\n", cur_if_cnt);
      gen(node->args[1]);

      // condition
      puts("pop rax");
      printf("cmp %s, 0\n", _rax(node->args[1]));
      printf("jne .Lbeginwork%d\n", cur_if_cnt);
      printf(".Lend%d:\n", cur_if_cnt);
      env_for_while = prev_env_for_while;
      env_for_while_switch = prev_env_for_while_switch;
      return;
   }

   if (node->ty == ND_IDENT || node->ty == ND_GLOBAL_IDENT || node->ty == '.' ||
       node->ty == ND_EXTERN_SYMBOL) {
      gen_lval(node);
      if (node->type->ty != TY_ARRAY) {
         puts("pop rax");
         printf("mov %s, [rax]\n", _rax(node));
         if (node->type->ty == TY_CHAR) {
            // TODO: extension: unsingned char.
            puts("movzx rax, al");
         }
         puts("push rax");
      }
      return;
   }

   if (node->ty == ND_DEREF) {
      gen(node->lhs); // Compile as RVALUE
      if (node->lhs->type->ptrof && node->lhs->type->ptrof->ty == TY_ARRAY) {
         return;
         // continuous array will be ignored.
      }
      puts("pop rax");
      printf("mov %s, [rax]\n", _rax(node));
      // when reading char, we should read just 1 byte
      if (node->type->ty == TY_CHAR) {
         printf("movzx rax, al\n");
      }
      puts("push rax");
      return;
   }

   if (node->ty == ND_NEG) {
      gen(node->lhs);
      puts("pop rax");
      printf("neg %s\n", _rax(node));
      puts("push rax");
      return;
   }

   if (node->ty == ND_ADDRESS) {
      gen_lval(node->lhs);
      return;
   }

   if (node->ty == ND_GOTO) {
      printf("jmp %s\n", node->name);
      return;
   }

   // read inside functions.
   if (node->ty == ND_RETURN) {
      if (node->lhs) {
         gen(node->lhs);
         puts("pop rax");
      }
      puts("mov rsp, rbp");
      puts("pop rbp");
      puts("ret");
      return;
   }

   if (node->ty == '=') {
      gen_lval(node->lhs);
      gen(node->rhs);
      puts("pop rdi");
      puts("pop rax");
      puts("mov [rax], rdi");
      // this should be rdi instead of edi
      puts("push rdi");
      return;
   }

   if (node->ty == ND_FPLUSPLUS) {
      gen_lval(node->lhs);
      puts("pop rax");
      puts("mov rdi, [rax]");
      puts("push rdi");
      printf("add rdi, %d\n", node->num_val);
      puts("mov [rax], rdi");
      return;
   }
   if (node->ty == ND_FSUBSUB) {
      gen_lval(node->lhs);
      puts("pop rax");
      puts("mov rdi, [rax]");
      puts("push rdi");
      printf("sub rdi, %d\n", node->num_val);
      puts("mov [rax], rdi");
      return;
   }

   if (node->ty == '!') {
      gen(node->lhs);
      puts("pop rax");
      printf("cmp %s, 0\n", _rax(node->lhs));
      puts("sete al");
      puts("movzx eax, al");
      puts("push rax");
      return;
   }

   gen(node->lhs);
   gen(node->rhs);

   puts("pop rdi"); // rhs
   puts("pop rax"); // lhs
   switch (node->ty) {
      case ',':
         puts("mov rax, rdi");
         break;
      case '+':
         printf("add %s, %s\n", _rax(node), _rdi(node));
         break;
      case '-':
         printf("sub %s, %s\n", _rax(node), _rdi(node));
         break;
      case '*':
         printf("mul rdi\n");
         break;
      case '/':
         puts("mov rdx, 0");
         puts("div rax, rdi");
         break;
      case '%':
         puts("mov rdx, 0");
         puts("div rax, rdi");
         // modulo are stored in rdx.
         puts("mov rax, rdx");
         break;
      case '^':
         printf("xor %s, %s\n", _rax(node), _rdi(node));
         break;
      case '&':
         printf("and %s, %s\n", _rax(node), _rdi(node));
         break;
      case '|':
         printf("or %s, %s\n", _rax(node), _rdi(node));
         break;
      case ND_RSHIFT:
         // FIXME: for signed int (Arthmetric)
         // mov rdi[8] -> rax
         // it should be 8-bited. (char)
         puts("mov cl, dil");
         puts("sar rax, cl");
         break;
      case ND_LSHIFT:
         puts("mov cl, dil");
         puts("sal rax, cl");
         break;
      case ND_ISEQ:
         cmp_rax_rdi(node);
         puts("sete al");
         puts("movzx rax, al");
         break;
      case ND_ISNOTEQ:
         cmp_rax_rdi(node);
         puts("setne al");
         puts("movzx rax, al");
         break;
      case '>':
         cmp_rax_rdi(node);
         puts("setg al");
         puts("movzx rax, al");
         break;
      case '<':
         cmp_rax_rdi(node);
         // TODO: is "andb 1 %al" required?
         puts("setl al");
         puts("movzx rax, al");
         break;
      case ND_ISMOREEQ:
         cmp_rax_rdi(node);
         puts("setge al");
         puts("and al, 1");
         // should be eax instead of rax?
         puts("movzx rax, al");
         break;
      case ND_ISLESSEQ:
         cmp_rax_rdi(node);
         puts("setle al");
         puts("and al, 1");
         puts("movzx eax, al");
         break;
      default:
         error("Error: no generations found.");
   }
   puts("push rax");
}

// TODO : removing this will cause error
int expect(int a);

Node *assign() {
   Node *node = node_mathexpr();
   if (consume_node('=')) {
      if (confirm_node(TK_OPAS)) {
         // FIXME: shift
         NodeType tp = tokens->data[pos]->input[0];
         expect_node(TK_OPAS);
         if (tokens->data[pos]->input[0] == '<') {
            tp = ND_LSHIFT;
         }
         if (tokens->data[pos]->input[0] == '>') {
            tp = ND_RSHIFT;
         }
         node = new_node('=', node, new_node(tp, node, assign()));
      } else {
         node = new_node('=', node, assign());
      }
   }
   return node;
}

Type *read_type(char **input) {
   Type *type = read_fundamental_type();
   if (!type) {
      error("Error: NOT a type when reading type.");
      return NULL;
   }
   // Variable Definition.
   // consume is pointer or not
   while (consume_node('*')) {
      Type *old_type = type;
      type = malloc(sizeof(Type));
      type->ty = TY_PTR;
      type->ptrof = old_type;
   }
   // There are input: there are ident names
   if (input) {
      *input = tokens->data[pos]->input;
   } else {
      input = &tokens->data[pos]->input;
   }
   // skip the name  of position.
   consume_node(TK_IDENT);
   // array
   if (consume_node('[')) {
      Type *base_type = type;
      type = malloc(sizeof(Type));
      type->ty = TY_ARRAY;
      type->array_size = (int)tokens->data[pos]->num_val;
      type->ptrof = base_type;
      expect_node(TK_NUM);
      expect_node(']');
      Type *cur_ptr = type;
      // support for multi-dimensional array
      while (consume_node('[')) {
         // TODO: support NOT functioned type
         // ex. int a[4+7];
         cur_ptr->ptrof = malloc(sizeof(Type));
         cur_ptr->ptrof->ty = TY_ARRAY;
         cur_ptr->ptrof->array_size = (int)tokens->data[pos]->num_val;
         cur_ptr->ptrof->ptrof = base_type;
         expect_node(TK_NUM);
         expect_node(']');
         cur_ptr = cur_ptr->ptrof;
      }
   }
   return type;
}

Node *stmt() {
   Node *node = NULL;
   if (confirm_type()) {
      char *input = NULL;
      Type *type = read_type(&input);
      node = new_ident_node_with_new_variable(input, type);
      // if there is int a =1;
      if (consume_node('=')) {
         if (consume_node('{')) {
         } else {
            node = new_node('=', node, node_mathexpr());
         }
      }
   } else if (consume_node(TK_RETURN)) {
      if (confirm_node(';')) {
         // to support return; with void type
         node = new_node(ND_RETURN, NULL, NULL);
      } else {
         node = new_node(ND_RETURN, assign(), NULL);
      }
      // FIXME GOTO is not statement, expr.
   } else if (consume_node(TK_GOTO)) {
      node = new_node(ND_GOTO, NULL, NULL);
      node->name = expect_ident();
   } else if (consume_node(TK_BREAK)) {
      node = new_node(ND_BREAK, NULL, NULL);
   } else if (consume_node(TK_CONTINUE)) {
      node = new_node(ND_CONTINUE, NULL, NULL);
   } else {
      node = assign();
   }
   if (!consume_node(';')) {
      error("Error: Not token ;");
   }
   return node;
}

Node *node_if() {
   Node *node = new_node(ND_IF, NULL, NULL);
   node->argc = 1;
   node->args[0] = assign();
   node->args[1] = NULL;
   // Suppress COndition

   if (confirm_node(TK_BLOCKBEGIN)) {
      node->lhs = new_block_node(env);
      program(node->lhs);
   } else {
      // without block
      node->lhs = stmt();
   }
   if (consume_node(TK_ELSE)) {
      if (confirm_node(TK_BLOCKBEGIN)) {
         node->rhs = new_block_node(env);
         program(node->rhs);
      } else if (consume_node(TK_IF)) {
         node->rhs = node_if();
      } else {
         node->rhs = stmt();
      }
   }
   return node;
}

void program(Node *block_node) {
   expect_node('{');
   Vector *args = block_node->code;
   Env *prev_env = env;
   env = block_node->env;
   while (!consume_node('}')) {
      if (confirm_node('{')) {
         Node *new_block = new_block_node(env);
         program(new_block);
         vec_push(args, new_block);
         continue;
      }

      if (consume_node(TK_IF)) {
         vec_push(args, node_if());
         continue;
      }
      if (consume_node(TK_WHILE)) {
         Node *while_node = new_node(ND_WHILE, node_mathexpr(), NULL);
         if (confirm_node(TK_BLOCKBEGIN)) {
            while_node->rhs = new_block_node(env);
            program(while_node->rhs);
         } else {
            while_node->rhs = stmt();
         }
         vec_push(args, while_node);
         continue;
      }
      if (consume_node(TK_DO)) {
         Node *do_node = new_node(ND_DOWHILE, NULL, NULL);
         do_node->rhs = new_block_node(env);
         program(do_node->rhs);
         expect_node(TK_WHILE);
         do_node->lhs = node_mathexpr();
         expect_node(';');
         vec_push(args, do_node);
         continue;
      }
      if (consume_node(TK_FOR)) {
         Node *for_node = new_node(ND_FOR, NULL, NULL);
         expect_node('(');
         // TODO: should be splited between definition and expression
         for_node->args[0] = stmt();
         for_node->rhs = new_block_node(env);
         // expect_node(';');
         // TODO: to allow without lines
         if (!consume_node(';')) {
            for_node->args[1] = assign();
            expect_node(';');
         }
         if (!consume_node(')')) {
            for_node->args[2] = assign();
            expect_node(')');
         }
         program(for_node->rhs);
         vec_push(args, for_node);
         continue;
      }

      if (consume_node(TK_CASE)) {
         vec_push(args, new_node(ND_CASE, node_term(), NULL));
         expect_node(':');
         continue;
      }
      if (consume_node(TK_DEFAULT)) {
         vec_push(args, new_node(ND_DEFAULT, NULL, NULL));
         expect_node(':');
         continue;
      }

      if (consume_node(TK_SWITCH)) {
         expect_node('(');
         Node *switch_node = new_node(ND_SWITCH, node_mathexpr(), NULL);
         expect_node(')');
         switch_node->rhs = new_block_node(env);
         program(switch_node->rhs);
         vec_push(args, switch_node);
         continue;
      }
      vec_push(args, stmt());
   }
   vec_push(args, NULL);

   // 'consumed }'
   env = prev_env;
}

// 0: neither 1:TK_TYPE 2:TK_IDENT

Type *find_typed_db(char *input, Map *db) {
   for (int j = 0; j < db->keys->len; j++) {
      // for struct
      if (strcmp(input, db->keys->data[j]) == 0) {
         // copy type
         Type *type = malloc(sizeof(Type));
         Type *old_type = (Type *)db->vals->data[j];
         // copy all
         type->ty = old_type->ty;
         type->structure = old_type->structure;
         type->array_size = old_type->array_size;
         type->ptrof = old_type->ptrof;
         type->offset = old_type->offset;
         return type;
      }
   }
   return NULL;
}

Type *read_fundamental_type() {
   if (tokens->data[pos]->ty == TK_CONST) {
      expect_node(TK_CONST); // TODO : for ease skip
   }
   if (tokens->data[pos]->ty == TK_ENUM) {
      // treat as anonymous enum
      expect_node(TK_ENUM);
      define_enum(0);
      return find_typed_db("int", typedb);
   }
   if (consume_node(TK_STRUCT)) {
      return find_typed_db(expect_ident(), struct_typedb);
   }
   return find_typed_db(expect_ident(), typedb);
}

int split_type_ident() {
   if (tokens->data[pos]->ty != TK_IDENT) {
      return 0;
   }
   for (int j = 0; j < typedb->keys->len; j++) {
      // for struct
      if (strcmp(tokens->data[pos]->input, typedb->keys->data[j]) == 0) {
         return typedb->vals->data[j]->ty;
      }
   }
   return 2; // IDENT
}

int confirm_type() {
   if (split_type_ident() > 2) {
      return 1;
   }
   return 0;
}
int confirm_ident() {
   if (split_type_ident() == 2) {
      return 1;
   }
   return 0;
}

char *expect_ident() {
   if (tokens->data[pos]->ty != TK_IDENT) {
      error("Error: Expected Ident but...");
      return NULL;
   }
   return tokens->data[pos++]->input;
}

void define_enum(int assign_name) {
   // ENUM def.
   consume_node(TK_IDENT); // for ease
   expect_node('{');
   Type *enumtype = malloc(sizeof(Type));
   enumtype->ty = TY_INT;
   enumtype->offset = 4;
   int cnt = 0;
   while (!consume_node('}')) {
      char *itemname = expect_ident();
      Node *itemnode = NULL;
      if (consume_node('=')) {
         itemnode = new_num_node(tokens->data[pos]->num_val);
         cnt = tokens->data[pos]->num_val;
         expect_node(TK_NUM);
      } else {
         itemnode = new_num_node(cnt);
      }
      consume_node(','); // TODO for ease
      cnt++;
      map_put(consts, itemname, itemnode);
   }
   // to support anonymous enum
   if (assign_name && confirm_ident()) {
      char *name = expect_ident();
      map_put(typedb, name, enumtype);
   }
}

void toplevel() {
   strcpy(registers[0], "rdi");
   strcpy(registers[1], "rsi");
   strcpy(registers[2], "rdx");
   strcpy(registers[3], "rcx");
   strcpy(registers[4], "r8");
   strcpy(registers[5], "r9");

   int i = 0;
   // consume_node('{')
   // idents = new_map...
   // stmt....
   // consume_node('}')
   global_vars = new_map();
   funcdefs = new_map();
   consts = new_map();
   strs = new_vector();
   env = new_env(NULL);

   while (!consume_node(TK_EOF)) {
      if (consume_node(TK_EXTERN)) {
         char *name = NULL;
         Type *type = read_type(&name);
         type->offset = -1; // TODO externed
         map_put(global_vars, name, type);
         expect_node(';');
         continue;
      }
      // definition of struct
      if (consume_node(TK_TYPEDEF)) {
         if (consume_node(TK_ENUM)) {
            // not anonymous enum
            define_enum(1);
            expect_node(';');
            continue;
         }
         Type *structuretype = malloc(sizeof(Type));
         if (consume_node(TK_STRUCT)) {
            if (confirm_node(TK_IDENT)) {
               map_put(struct_typedb, expect_ident(), structuretype);
            }
         }
         expect_node('{');
         structuretype->structure = new_map();
         structuretype->ty = TY_STRUCT;
         structuretype->ptrof = NULL;
         int offset = 0;
         int size = 0;
         while (!consume_node('}')) {
            char *name = NULL;
            Type *type = read_type(&name);
            size = type2size(type);
            if ((offset % size != 0)) {
               offset += (size - offset % size);
            }
            // all type should aligned with proper value.
            // TODO assumption there are NO bytes over 8 bytes.
            type->offset = offset;
            offset += cnt_size(type);
            expect_node(';');
            map_put(structuretype->structure, name, type);
         }
         structuretype->offset = offset;
         char *name = expect_ident();
         expect_node(';');
         map_put(typedb, name, structuretype);
         continue;
      }

      if (consume_node(TK_ENUM)) {
         consume_node(TK_IDENT); // for ease
         expect_node('{');
         char *name = expect_ident();
         Type *structuretype = malloc(sizeof(Type));
         expect_node(';');
         map_put(typedb, name, structuretype);
      }

      if (confirm_type()) {
         char *name = NULL;
         Type *type = read_type(&name);
         if (consume_node('(')) {
            code[i] = new_fdef_node(name, env, type);
            // Function definition because toplevel func call

            // TODO env should be treated as cooler bc. of splitted namespaces
            Env *prev_env = env;
            env = code[i]->env;
            for (code[i]->argc = 0; code[i]->argc <= 6 && !consume_node(')');) {
               char *arg_name = NULL;
               Type *arg_type = read_type(&arg_name);
               code[i]->args[code[i]->argc++] =
                   new_ident_node_with_new_variable(arg_name, arg_type);
               consume_node(',');
            }
            env = prev_env;
            // to support prototype def.
            if (confirm_node('{')) {
               program(code[i++]);
            } else {
               expect_node(';');
            }
            continue;
         } else {
            // Global Variables.
            type->offset = 0; // TODO externed
            map_put(global_vars, name, type);
            type->initval = 0;
            if (consume_node('=')) {
               Node *initval = node_term();
               // TODO: only supported main valu.
               type->initval = initval->num_val;
            }
            expect_node(';');
         }
         continue;
      }
      if (confirm_node('{')) {
         code[i] = new_block_node(NULL);
         program(code[i++]);
         continue;
      }
      code[i] = stmt();
      i++;
   }
   code[i] = NULL;
}

void test_map() {
   Vector *vec = new_vector();
   Token *hanando_fukui_compiled = malloc(sizeof(Token));
   hanando_fukui_compiled->ty = TK_NUM;
   hanando_fukui_compiled->pos = 0;
   hanando_fukui_compiled->num_val = 1;
   hanando_fukui_compiled->input = "HANANDO_FUKUI";
   vec_push(vec, hanando_fukui_compiled);
   vec_push(vec, 9);
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
   if ((int)map_get(map, "bar") != 0) {
      error("Error: Map does not work yet! on 3");
   }
   Token *te = map_get(map, "foo");
   if (strcmp(te->input, "HANANDO_FUKUI") != 0) {
      error("Error: Map does not work yet! on 3a");
   }
}

void globalvar_gen() {
   puts(".data");
   for (int j = 0; j < global_vars->keys->len; j++) {
      Type *valdataj = (Type *)global_vars->vals->data[j];
      char *keydataj = (char *)global_vars->keys->data[j];
      if (valdataj->offset < 0) {
         // There are no offset. externed.
         continue;
      }
      if (valdataj->initval != 0) {
         printf(".type %s,@object\n", keydataj);
         printf(".global %s\n", keydataj);
         printf(".size %s, %d\n", keydataj, cnt_size(valdataj));
         printf("%s:\n", keydataj);
         printf(".long %d\n", valdataj->initval);
         puts(".text");
      } else {
         printf(".type %s,@object\n", keydataj);
         printf(".global %s\n", keydataj);
         printf(".comm %s, %d\n", keydataj, cnt_size(valdataj));
      }
   }
   for (int j = 0; j < strs->len; j++) {
      printf(".LC%d:\n", j);
      printf(".string \"%s\"\n", (char *)strs->data[j]);
   }
}

void preprocess(Vector *pre_tokens) {
   Map *defined = new_map();
   // when compiled with hanando
   Token *hanando_fukui_compiled = malloc(sizeof(Token));
   hanando_fukui_compiled->ty = TK_NUM;
   hanando_fukui_compiled->pos = 0;
   hanando_fukui_compiled->num_val = 1;
   hanando_fukui_compiled->input = "__HANANDO_FUKUI__";
   map_put(defined, "__HANANDO_FUKUI__", hanando_fukui_compiled);

   int skipped = 0;
   for (int j = 0; j < pre_tokens->len; j++) {
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
               preprocess(read_tokenize(pre_tokens->data[j + 2]->input));
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
      for (int k = 0; k < defined->keys->len; k++) {
         char *chr = defined->keys->data[k];
         if (pre_tokens->data[j]->ty == TK_IDENT &&
             strcmp(pre_tokens->data[j]->input, chr) == 0) {
            called = 1;
            printf("#define changed: %s -> %ld\n", pre_tokens->data[j]->input,
                   defined->vals->data[k]->num_val);
            vec_push(tokens, defined->vals->data[k]);
            continue;
         }
      }
      if (!called) {
         vec_push(tokens, pre_tokens->data[j]);
      }
   }
}

void init_typedb() {
   enum_typedb = new_map();
   struct_typedb = new_map();
   typedb = new_map();

   Type *typeint = malloc(sizeof(Type));
   typeint->ty = TY_INT;
   typeint->ptrof = NULL;
   map_put(typedb, "int", typeint);

   Type *typechar = malloc(sizeof(Type));
   typechar->ty = TY_CHAR;
   typechar->ptrof = NULL;
   map_put(typedb, "char", typechar);

   Type *typelong = malloc(sizeof(Type));
   typelong->ty = TY_LONG;
   typelong->ptrof = NULL;
   map_put(typedb, "long", typelong);

   Type *typevoid = malloc(sizeof(Type));
   typevoid->ty = TY_VOID;
   typevoid->ptrof = NULL;
   map_put(typedb, "void", typevoid);

   typevoid = malloc(sizeof(Type));
   typevoid->ty = TY_STRUCT;
   typevoid->ptrof = NULL;
   typevoid->offset = 8;
   typevoid->structure = new_map();
   map_put(typedb, "FILE", typevoid);

   Type *typedou = malloc(sizeof(Type));
   typedou->ty = TY_DOUBLE;
   typedou->ptrof = NULL;
   typedou->offset = 8;
   map_put(typedb, "double", typevoid);
}

Vector *read_tokenize(char *fname) {
   FILE *fp = fopen(fname, "r");
   if (!fp) {
      fprintf(stderr, "No file found: %s\n", fname);
      exit(1);
   }
   fseek(fp, 0, SEEK_END);
   long length = ftell(fp);
   fseek(fp, 0, SEEK_SET);
   char *buf = malloc(sizeof(char) * (length + 5));
   fread(buf, length + 5, sizeof(char), fp);
   fclose(fp);
   return tokenize(buf);
}

int main(int argc, char **argv) {
   if (argc < 2) {
      error("Incorrect Arguments");
   }
   test_map();

   tokens = new_vector();
   if (strcmp(argv[1], "-f") == 0) {
      preprocess(read_tokenize(argv[2]));
   } else {
      preprocess(tokenize(argv[argc - 1]));
   }

   init_typedb();

   toplevel();

   puts(".intel_syntax noprefix");
   puts(".align 4");
   // treat with global variables
   globalvar_gen();

   // TODO support ./hanando -r -f main.c
   if (strcmp(argv[1], "-r") == 0) {
      gen_register_top();
   } else {
      for (int j = 0; code[j]; j++) {
         gen(code[j]);
      }
   }

   return 0;
}
