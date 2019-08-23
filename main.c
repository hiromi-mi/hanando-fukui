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

char *strdup(const char *s);

// float ceil(float x);
// to use vector instead of something
Vector *tokens;
int pos = 0;
Vector *globalcode;
Map *global_vars;
Map *funcdefs_generated_template;
Map *funcdefs;
Map *consts;
Vector *strs;
Map *typedb;
Map *struct_typedb;
Map *enum_typedb;
Map *current_local_typedb;
int is_recursive(Node *node, char *name);

int env_for_while = 0;
int env_for_while_switch = 0;
Env *env;
int if_cnt = 0;
int for_while_cnt = 0;
int omiited_argc = 0; // when used va_start & TK_OMIITED

char arg_registers[6][4];

int lang = 0;

Node *new_node(NodeType ty, Node *lhs, Node *rhs);
int type2size(Type *type);
Type *read_type_all(char **input);
Type *read_type(Type *type, char **input, Map *local_typedb);
Type *read_fundamental_type(Map *local_typedb);
int confirm_type();
int confirm_node(TokenConst ty);
int confirm_ident();
int consume_ident();
int split_type_caller();
Vector *read_tokenize(char *fname);
void define_enum(int use);
char *expect_ident();
void program(Node *block_node);
Type *duplicate_type(Type *old_type);
Type *copy_type(Type *old_type, Type *type);
Type *find_typed_db(char *input, Map *db);
int cnt_size(Type *type);
Type *get_type_local(Node *node);
Node *new_addsub_node(NodeType ty, Node *lhs, Node *rhs);
Node *generate_template(Node *node, Map *template_type_db);

Register *gen_register_rightval(Node *node, int unused_eval);
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

char *type2name(Type *type) {
   char *buf = malloc(sizeof(char) * 256);
   switch (type->ty) {
      case TY_INT:
         return "int";
      case TY_CHAR:
         return "char";
      case TY_FLOAT:
         return "float";
      case TY_PTR:
      case TY_ARRAY:
         sprintf(buf, "%s_ptr", type2name(type->ptrof));
         return buf;
      case TY_STRUCT:
         if (type->name) {
            sprintf(buf, "%s_struct", type->name);
         } else {
            sprintf(buf, "nonname_struct");
         }
         return buf;
      default:
         return "type";
   }
}

Type *new_type() {
   Type *type = malloc(sizeof(Type));
   type->ptrof = NULL;
   type->ret = NULL;
   type->argc = 0;
   type->array_size = 0;
   type->initval = 0;
   type->offset = 0;
   type->is_const = 0;
   type->is_static = 0;
   type->is_omiited = 0;
   type->name = NULL;
   type->context = malloc(sizeof(Type));
   type->context->is_previous_class = NULL;
   type->context->method_name = NULL;
   type->local_typedb = NULL;
   type->template_name = NULL;
   return type;
}

Type *type_pointer(Type *base_type) {
   Type *type = new_type();
   type->ty = TY_PTR;
   type->ptrof = base_type;
   return type;
}

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
      if (strcmp((char *)map->keys->data[i], key) == 0) {
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
      fprintf(stderr, "%s on line %d pos %d: %s\n", str,
              tokens->data[pos]->pline, pos, tokens->data[pos]->input);
   } else {
      fprintf(stderr, "%s\n", str);
   }
   exit(1);
}

Node *new_string_node(char *_id) {
   Node *_node = new_node(ND_STRING, NULL, NULL);
   _node->name = _id;
   Type *type = new_type();
   type->ty = TY_CHAR;
   _node->type = type_pointer(type);
   _node->pline = -1;
   return _node;
}

Node *new_node(NodeType ty, Node *lhs, Node *rhs) {
   Node *node = malloc(sizeof(Node));
   node->ty = ty;
   node->lhs = lhs;
   node->rhs = rhs;
   node->conds[0] = NULL;
   node->conds[1] = NULL;
   node->conds[2] = NULL;

   node->code = NULL;
   node->argc = 0;
   node->num_val = 0;
   node->float_val = 0;
   node->name = NULL;
   node->gen_name = NULL;
   node->env = NULL;
   node->type = NULL;
   node->lvar_offset = 0;
   node->is_omiited = NULL;
   node->is_static = 0;
   node->is_recursive = 0;
   node->args[0] = NULL;
   node->args[1] = NULL;
   node->args[2] = NULL;
   node->args[3] = NULL;
   node->args[4] = NULL;
   node->args[5] = NULL;
   node->pline = -1; // TODO for more advenced text
   node->funcdef = NULL;

   if (lhs) {
      node->type = node->lhs->type;
   }
   return node;
}

Node *new_node_with_cast(NodeType ty, Node *lhs, Node *rhs) {
   return new_node(ty, lhs, rhs);
}

Node *new_assign_node(Node *lhs, Node *rhs) {
   return new_node(ND_ASSIGN, lhs, rhs);
}
Node *new_char_node(long num_val) {
   Node *node = new_node(ND_NUM, NULL, NULL);
   node->num_val = num_val;
   node->type = find_typed_db("char", typedb);
   node->pline = -1; // TODO for more advenced textj
   return node;
}

Node *new_num_node_from_token(Token *token) {
   Node *node = new_node(ND_NUM, NULL, NULL);
   node->num_val = token->num_val;
   node->pline = -1; // TODO for more advenced textj
   switch (token->type_size) {
      case 1:
         node->type = find_typed_db("char", typedb);
         return node;
      case 4:
         node->type = find_typed_db("int", typedb);
         return node;
      default:
         // including case 8:
         node->type = find_typed_db("long", typedb);
         return node;
   }
}

Node *new_num_node(long num_val) {
   Node *node = new_node(ND_NUM, NULL, NULL);
   node->num_val = num_val;
   node->pline = -1; // TODO for more advenced textj
   node->type = find_typed_db("int", typedb);
   return node;
}
Node *new_long_num_node(long num_val) {
   Node *node = new_node(ND_NUM, NULL, NULL);
   node->ty = ND_NUM;
   node->num_val = num_val;
   node->pline = -1; // TODO for more advenced textj
   node->type = find_typed_db("long", typedb);
   return node;
}
Node *new_float_node(float num_val) {
   Node *node = new_node(ND_FLOAT, NULL, NULL);
   node->ty = ND_FLOAT;
   node->pline = -1; // TODO for more advenced textj
   node->num_val = num_val;
   node->type = find_typed_db("float", typedb);
   return node;
}

Node *new_deref_node(Node *lhs) {
   Node *node = new_node(ND_DEREF, lhs, NULL);
   // moved to new_deref_node
   if (node->type->ty != TY_TEMPLATE) {
      node->type = node->lhs->type->ptrof;
      if (!node->type) {
         error("Error: Dereference on NOT pointered.");
      }
   }
   node->pline = -1; // TODO for more advenced textj
   return node;
}

Node *new_addsub_node(NodeType ty, Node *lhs, Node *rhs) {
   return new_node(ty, lhs, rhs);
}

void update_rsp_offset(int size) {
   env->rsp_offset += size;
   if (*env->rsp_offset_max < env->rsp_offset) {
      *env->rsp_offset_max = env->rsp_offset;
   }
}

Node *new_ident_node_with_new_variable(char *name, Type *type) {
   Node *node = new_node(ND_IDENT, NULL, NULL);
   node->name = name;
   node->type = type;
   node->pline = -1; // TODO for more advenced textj
   int size = cnt_size(type);
   // should aligned as x86_64
   if (size % 8 != 0) {
      size += (8 - size % 8);
   }
   update_rsp_offset(size);
   node->lvar_offset = env->rsp_offset;
   type->offset = env->rsp_offset;
   map_put(env->idents, name, type);
   /*
   if ((lang & 1) && type->ty == TY_STRUCT) {
      // On Class, Default Constructor Should Be Called
      Type *typed = map_get(type->structure, name);
      if (typed) {
         // Call Constructor
         return new_func_node(typed);
      }
   }
   */
   return node;
}

Node *new_ident_node(char *name) {
   Node *node = new_node(ND_IDENT, NULL, NULL);
   node->name = name;
   node->pline = -1; // TODO for more advenced textj
   if (strcmp(node->name, "stderr") == 0) {
      // TODO dirty
      node->ty = ND_EXTERN_SYMBOL;
      node->type = new_type();
      node->type->ty = TY_PTR;
      node->type->ptrof = new_type();
      node->type->ptrof->ty = TY_INT;
      return node;
   }
   node->type = get_type_local(node);
   if (node->type) {
      node->lvar_offset = node->type->offset;
      return node;
   }

   // Try Global Function.
   Node *result = map_get(funcdefs, name);
   if (result) {
      node->type = result->type;
      node->ty = ND_SYMBOL;
      return node;
   }
   // If expect this is an (unknown) function
   if (confirm_node('(')) {
      node->ty = ND_SYMBOL;
      node->type = find_typed_db("int", typedb);
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

char *mangle_func_name(char *name) {
   char *p;
   char *buf = malloc(sizeof(char) * 256);
   char *q = buf;
   // Mangle
   if ((lang & 1)) {
      p = name;
      // C++
      while (*p != '\0') {
         if (*p == ':' && *(p + 1) == ':') {
            strncpy(q, "MangleD", 8);
            p += 2;
            q += 7;
            continue;
         }
         *q = *p;
         p++;
         q++;
      }
      *q = '\0';
      return buf;
   } else {
      return name;
   }
}

Node *new_func_node(Node *ident, Vector *template_types) {
   Node *node = new_node(ND_FUNC, ident, NULL);
   Map *template_type_db = new_map();
   node->type = NULL;
   node->argc = 0;
   Type *template_type = NULL;
   int j;
   char *name = malloc(sizeof(char) * 256);
   node->pline = -1; // TODO for more advenced textj
   if (ident->ty == ND_SYMBOL || ident->ty == ND_DOT) {
      if (ident->ty == ND_SYMBOL) {
         name = ident->name;
      } else if (ident->ty == ND_DOT) {
         sprintf(name, "%s::%s", ident->lhs->type->name, ident->name);
      }
      node->name = name;
      node->gen_name = mangle_func_name(node->name);
      Node *result = map_get(funcdefs, node->name);
      if (result) {
         node->funcdef = result;
         node->type = result->type->ret;

         // use the function definition.
         if (ident->ty == ND_DOT) {
            // TODO: Dirty: Add "this"
            node->args[0] = new_node(ND_ADDRESS, ident->lhs, NULL);
            node->argc = 1;
         }
         if (template_types) {
            if (template_types->len != result->type->local_typedb->keys->len) {
               error("Error: The number of template arguments does not match.");
            }
            // Make connection from template_types to local_typedb

            char *buf = malloc(sizeof(char) * 1024);
            sprintf(buf, "%s", ident->name);
            for (j = 0; j < result->type->local_typedb->keys->len; j++) {
               template_type =
                   (Type *)result->type->local_typedb->vals->data[j];
               map_put(template_type_db, template_type->template_name,
                       template_types->data[j]);
               strncat(buf, "_template_", 12);
               strncat(buf, type2name((Type *)template_types->data[j]), 128);
            }
            node->gen_name = buf;
            if (!map_get(funcdefs_generated_template, buf)) {
               result = generate_template(result, template_type_db);
               // specialize on local var.
               node = generate_template(node, template_type_db);
               result->gen_name = buf;
               map_put(funcdefs_generated_template, result->gen_name, result);
               vec_push(globalcode, (Token *)result);
            }
         }
      } else {
         node->type = find_typed_db("int", typedb);
      }
   } else {
      // something too complex
      // so add into function node & call
      node->name = NULL;
      node->gen_name = NULL;
      // FIXME this should be type
      node->type = ident->type;
   }
   return node;
}

Env *new_env(Env *prev_env) {
   Env *_env = malloc(sizeof(Env));
   _env->env = prev_env;
   _env->idents = new_map();
   _env->rsp_offset = 0;
   if (prev_env) {
      _env->rsp_offset = prev_env->rsp_offset;
      _env->rsp_offset_max = prev_env->rsp_offset_max;
   } else {
      _env->rsp_offset_max = malloc(sizeof(int));
      *_env->rsp_offset_max = 0;
   }
   return _env;
}

Node *new_fdef_node(char *name, Type *type, int is_static) {
   Node *node = new_node(ND_FDEF, NULL, NULL);
   node->type = type;
   node->name = name;
   node->argc = 0;
   node->env = new_env(NULL);
   node->code = new_vector();
   node->is_omiited = NULL; // func(a, b, ...)
   node->is_static = is_static;
   node->gen_name = mangle_func_name(name);
   node->pline = -1; // TODO for more advenced textj
   map_put(funcdefs, name, node);
   return node;
}

Node *new_block_node(Env *prev_env) {
   Node *node = new_node(ND_BLOCK, NULL, NULL);
   node->pline = -1; // TODO for more advenced textj
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
            vec_push(pre_tokens, new_token(pline, TK_LSHIFT, p));
            p += 2;
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
            vec_push(pre_tokens, new_token(pline, TK_RSHIFT, p));
            p += 2;
         } else if (*(p + 1) == '=') {
            vec_push(pre_tokens, new_token(pline, TK_ISMOREEQ, p));
            p += 2;
         } else {
            vec_push(pre_tokens, new_token(pline, *p, p));
            p++;
         }
         continue;
      }
      if (isdigit(*p)) {
         Token *token = new_token(pline, TK_NUM, p);
         token->num_val = strtol(p, &p, 10);
         token->type_size = 4; // to treat as int

         // if there are FLOAT
         if (*p == '.') {
            token->ty = TK_FLOAT;
            token->num_val = strtof(token->input, &p);
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
         } else if (strcmp(token->input, "class") == 0) {
            token->ty = TK_CLASS;
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
         } else if (strcmp(token->input, "public") == 0) {
            token->ty = TK_PUBLIC;
         } else if (strcmp(token->input, "private") == 0) {
            token->ty = TK_PRIVATE;
         } else if (strcmp(token->input, "template") == 0) {
            token->ty = TK_TEMPLATE;
         } else if (strcmp(token->input, "typename") == 0) {
            token->ty = TK_TYPENAME;
         } else if (strcmp(token->input, "__LINE__") == 0) {
            token->ty = TK_NUM;
            token->num_val = pline;
            token->type_size = 8;
         }

         vec_push(pre_tokens, token);
         continue;
      }

      fprintf(stderr, "Cannot Tokenize: %s\n", p);
      exit(1);
   }

   vec_push(pre_tokens, new_token(pline, TK_EOF, p));
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
         node = new_node_with_cast('<', node, node_shift());
         node->type = find_typed_db("char", typedb);
      } else if (consume_node('>')) {
         node = new_node_with_cast('>', node, node_shift());
         node->type = find_typed_db("char", typedb);
      } else if (consume_node(TK_ISLESSEQ)) {
         node = new_node_with_cast(ND_ISLESSEQ, node, node_shift());
         node->type = find_typed_db("char", typedb);
      } else if (consume_node(TK_ISMOREEQ)) {
         node = new_node_with_cast(ND_ISMOREEQ, node, node_shift());
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
         node = new_node_with_cast(ND_ISEQ, node, node_compare());
         node->type = find_typed_db("char", typedb);
      } else if (consume_node(TK_ISNOTEQ)) {
         node = new_node_with_cast(ND_ISNOTEQ, node, node_compare());
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
         Type *type = read_type_all(NULL);
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
   if (consume_node('-')) {
      Node *node = new_node(ND_NEG, node_increment(), NULL);
      return node;
   } else if (consume_node('!')) {
      Node *node = new_node('!', node_increment(), NULL);
      return node;
   } else if (consume_node('+')) {
      Node *node = node_increment();
      return node;
   } else if (consume_node(TK_PLUSPLUS)) {
      node = new_ident_node(expect_ident());
      node = new_assign_node(node, new_addsub_node('+', node, new_num_node(1)));
   } else if (consume_node(TK_SUBSUB)) {
      node = new_ident_node(expect_ident());
      node = new_assign_node(node, new_addsub_node('-', node, new_num_node(1)));
   } else if (consume_node('&')) {
      node = node_increment();
      if (node->ty == ND_DEREF) {
         node = node->lhs;
      } else {
         node = new_node(ND_ADDRESS, node, NULL);
         node->type = new_type();
         node->type->ty = TY_PTR;
         node->type->ptrof = node->lhs->type;
      }
   } else if (consume_node('*')) {
      node = new_deref_node(node_increment());
   } else if (consume_node(TK_SIZEOF)) {
      if (consume_node('(') && confirm_type()) {
         // sizeof(int) read type without name
         Type *type = read_type_all(NULL);
         expect_node(')');
         return new_num_node(cnt_size(type));
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
         node = new_node_with_cast('*', node, node_cast());
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
   if (node->type->ty != TY_TEMPLATE) {
      node->type = (Type *)map_get(node->lhs->type->structure, node->name);
      if (!node->type) {
         error("Error: structure not found.");
      }
   }
   // Member Accesss Control p.231
   if ((lang & 1) && (node->type->memaccess == PRIVATE)) {
      // FIXME : to implement PRIVATE & PUBLIC
   }
   return node;
}

Node *treat_va_start() {
   Node *node = new_node(ND_VASTART, NULL, NULL);
   expect_node('(');
   node->lhs = node_term();
   node->rhs = new_ident_node("_saved_var");
   expect_node(',');
   node_term();
   // darkness: used global variable.
   node->num_val = omiited_argc;
   expect_node(')');
   return node;
}

Node *treat_va_arg() {
   Node *node = new_node(ND_VAARG, NULL, NULL);
   expect_node('(');
   node->lhs = node_term();
   expect_node(',');
   // do not need ident name
   node->type = read_type_all(NULL);
   expect_node(')');
   return node;
}

Node *treat_va_end() {
   Node *node = new_node(ND_VAEND, NULL, NULL);
   expect_node('(');
   node->lhs = node_term();
   expect_node(')');
   return node;
}

Node *node_term() {
   Node *node;
   // Primary Expression
   if (confirm_type(TK_FLOAT)) {
      node = new_float_node((float)tokens->data[pos]->num_val);
      expect_node(TK_NUM);
   } else if (confirm_node(TK_NUM)) {
      node = new_num_node(tokens->data[pos]->num_val);
      expect_node(TK_NUM);
   } else if (consume_node(TK_NULL)) {
      node = new_num_node(0);
      node->type->ty = TY_PTR;
   } else if (confirm_ident()) {
      // treat constant or variable
      char *input = expect_ident();
      for (int j = 0; j < consts->keys->len; j++) {
         // support for constant
         if (strcmp((char *)consts->keys->data[j], input) == 0) {
            node = (Node *)consts->vals->data[j];
            return node;
         }
      }
      node = new_ident_node(input);
   } else if (consume_node('(')) {
      node = assign();
      if (consume_node(')') == 0) {
         error("Error: Incorrect Parensis.");
      }
   } else if (confirm_node(TK_STRING)) {
      char *_str = malloc(sizeof(char) * 256);
      snprintf(_str, 255, ".LC%d",
               vec_push(strs, (Token *)tokens->data[pos]->input));
      node = new_string_node(_str);
      expect_node(TK_STRING);
   }

   Vector *template_types = NULL;
   Type *template_type = NULL;
   while (1) {
      // Postfix Expression

      // Template
      if ((lang & 1) && confirm_node('<')) {
         template_types = new_vector();
         if (node->ty != ND_SYMBOL) {
            continue;
         }
         Node *result = map_get(funcdefs, node->name);
         if (!result || !result->type->local_typedb ||
             result->type->local_typedb->keys->len <= 0) {
            continue;
         }
         expect_node('<');
         while (1) {
            template_type = read_fundamental_type(NULL);
            template_type = read_type(template_type, NULL, NULL);
            // In this position, we don't know definition of template_type_db
            vec_push(template_types, (Token *)template_type);
            if (!consume_node(',')) {
               expect_node('>');
               break;
            }
         }
      } else if (confirm_node('(')) {
         // Function Call
         // char *fname = expect_ident();
         char *fname = "";
         if (node->ty == ND_SYMBOL) {
            fname = node->name;
         }
         if (strncmp(fname, "va_arg", 7) == 0) {
            return treat_va_arg();
         } else if (strncmp(fname, "va_start", 8) == 0) {
            return treat_va_start();
         } else if (strncmp(fname, "va_end", 7) == 0) {
            return treat_va_end();
         }

         node = new_func_node(node, template_types);
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
         // return node;
      } else if (consume_node(TK_PLUSPLUS)) {
         node = new_node(ND_FPLUSPLUS, node, NULL);
         node->num_val = 1;
         // if (node->lhs->type->ty == TY_PTR || node->lhs->type->ty ==
         // TY_ARRAY) { Moved to analyzing process.
      } else if (consume_node(TK_SUBSUB)) {
         node = new_node(ND_FSUBSUB, node, NULL);
         node->num_val = 1;
         // if (node->lhs->type->ty == TY_PTR || node->lhs->type->ty ==
         // TY_ARRAY) { Moved to analyzing process.
      } else if (consume_node('[')) {
         node = new_deref_node(new_addsub_node('+', node, node_mathexpr()));
         expect_node(']');
      } else if (consume_node('.')) {
         node = new_dot_node(node);
      } else if (consume_node(TK_ARROW)) {
         node = new_dot_node(new_deref_node(node));
      } else {
         return node;
      }
   }
   /*
   if (pos > 0) {
      fprintf(stderr, "Error: Incorrect Paresis without %c %d -> %c %d\n",
            tokens->data[pos - 1]->ty, tokens->data[pos - 1]->ty,
            tokens->data[pos]->ty, tokens->data[pos]->ty);
   } else {
      fprintf(stderr, "Error: Incorrect Paresis without\n");
   }
   exit(1);
*/
}

Type *get_type_local(Node *node) {
   Env *local_env = env;
   Type *type = NULL;
   while (type == NULL && local_env != NULL) {
      type = map_get(local_env->idents, node->name);
      if (type) {
         node->env = local_env;
         return type;
      }
      local_env = local_env->env;
   }
   return type;
}

char *_rax(Node *node) {
   switch (type2size(node->type)) {
      case 1:
         return "al";
      case 4:
         return "eax";
      default:
         return "rax";
   }
}

char *_rdi(Node *node) {
   switch (type2size(node->type)) {
      case 1:
         return "dil";
      case 4:
         return "edi";
      default:
         return "rdi";
   }
}

char *_rdx(Node *node) {
   switch (type2size(node->type)) {
      case 1:
         return "dl";
      case 4:
         return "edx";
      default:
         return "rdx";
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

int type2size3(Type *type) {
   if (!type) {
      return 0;
   }
   switch (type->ty) {
      case TY_ARRAY:
         return cnt_size(type->ptrof);
      case TY_FUNC:
         return 0;
      default:
         return type2size(type);
   }
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
      case TY_FLOAT:
         return 4;
      case TY_CHAR:
         return 1;
      case TY_ARRAY:
         return 8;
      case TY_STRUCT:
         return cnt_size(type);
      case TY_FUNC:
         return 8;
      default:
         error("Error: NOT a type");
         return 0;
   }
}

int cnt_size(Type *type) {
   switch (type->ty) {
      case TY_PTR:
      case TY_INT:
      case TY_FLOAT:
      case TY_CHAR:
      case TY_LONG:
      case TY_FUNC:
         return type2size(type);
      case TY_ARRAY:
         return cnt_size(type->ptrof) * type->array_size;
      case TY_STRUCT:
         return type->offset;
      case TY_TEMPLATE:
         return 8; // FIXME
      default:
         error("Error: on void type error.");
         return 0;
   }
}

int reg_table[6];
int float_reg_table[8];
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

char *gvar_size2reg(int size, char *name) {
   char *_str = malloc(sizeof(char) * 256);
   if (size == 1) {
      snprintf(_str, 255, "byte ptr %s[rip]", name);
   } else if (size == 4) {
      snprintf(_str, 255, "dword ptr %s[rip]", name);
   } else {
      snprintf(_str, 255, "qword ptr %s[rip]", name);
   }
   return _str;
}

char *node2specifier(Node *node) {
   int size;
   size = type2size(node->type);
   if (size == 1) {
      return "byte ptr";
   } else if (size == 4) {
      return "dword ptr";
   } else {
      return "qword ptr";
   }
}

char *gvar_node2reg(Node *node, char *name) {
   return gvar_size2reg(type2size(node->type), name);
}

char *size2reg(int size, Register *reg) {
   if (reg->kind == R_REGISTER) {
      if (size == 1) {
         return id2reg8(reg->id);
      } else if (size == 4) {
         return id2reg32(reg->id);
      } else {
         return id2reg64(reg->id);
      }
      // with type specifier.
   } else if (reg->kind == R_LVAR) {
      char *_str = malloc(sizeof(char) * 256);
      if (size == 1) {
         snprintf(_str, 255, "byte ptr [rbp-%d]", reg->id);
      } else if (size == 4) {
         snprintf(_str, 255, "dword ptr [rbp-%d]", reg->id);
      } else {
         snprintf(_str, 255, "qword ptr [rbp-%d]", reg->id);
      }
      return _str;
   } else if (reg->kind == R_GVAR) {
      return gvar_size2reg(size, reg->name);
   }
   fprintf(stderr, "Error: Cannot Have Register\n");
   exit(1);
}
char *node2reg(Node *node, Register *reg) {
   if (reg->kind == R_LVAR) {
      return size2reg(reg->size, reg);
   }
   return size2reg(type2size(node->type), reg);
}

Register *float_retain_reg() {
   for (int j=0;j<8;j++) {
      if (float_reg_table[j] > 0)
         continue;
      float_reg_table[j] = 1;

      Register *reg = malloc(sizeof(Register));
      reg->id = j;
      reg->name = NULL;
      reg->kind = R_XMM;
   }
   fprintf(stderr, "No more float registers are avaliable\n");
   exit(1);
}

Register *retain_reg() {
   for (int j = 0; j < 6; j++) {
      if (reg_table[j] > 0)
         continue;
      reg_table[j] = 1;

      Register *reg = malloc(sizeof(Register));
      reg->id = j;
      reg->name = NULL;
      reg->kind = R_REGISTER;
      reg->size = -1;
      return reg;
   }
   fprintf(stderr, "No more registers are avaliable\n");
   exit(1);
}

void release_reg(Register *reg) {
   if (reg->kind == R_REGISTER) {
      reg_table[reg->id] = 0;
   } else if (reg->kind == R_XMM) {
      float_reg_table[reg->id] = 0;
   }
   free(reg);
}

void release_all_reg() {
   for (int j = 0; j < 6; j++) {
      reg_table[j] = 0;
   }
   for (int j = 0; j < 8; j++) {
      float_reg_table[j] = 0;
   }
}

void secure_mutable(Register *reg) {
   // to enable to change
   if (reg->kind != R_REGISTER && reg->kind != R_XMM) {
      Register *new_reg = retain_reg();
      if (reg->size <= 0) {
         printf("mov %s, %s\n", size2reg(8, new_reg), size2reg(8, reg));
      } else if (reg->size == 1) {
         printf("movzx %s, %s\n", size2reg(8, new_reg),
                size2reg(reg->size, reg));
      } else {
         printf("mov %s, %s\n", size2reg(reg->size, new_reg),
                size2reg(reg->size, reg));
      }
      reg->id = new_reg->id;
      reg->kind = new_reg->kind;
      reg->name = new_reg->name;
      reg->size = new_reg->size;
   }
}

Register *gen_register_leftval(Node *node) {
   Register *temp_reg;
   Register *lhs_reg;
   // Treat as lvalue.
   if (!node) {
      return NO_REGISTER;
   }
   switch (node->ty) {
      case ND_IDENT:
         temp_reg = retain_reg();
         printf("lea %s, [rbp-%d]\n", id2reg64(temp_reg->id),
                node->lvar_offset);
         return temp_reg;

      case ND_DEREF:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         if (lhs_reg->kind != R_REGISTER && lhs_reg->kind != R_XMM) {
            temp_reg = retain_reg();
            printf("mov %s, %s\n", size2reg(8, temp_reg),
                   node2reg(node->lhs, lhs_reg));
            release_reg(lhs_reg);
            lhs_reg = temp_reg;
         }
         return lhs_reg;

      case ND_GLOBAL_IDENT:
      case ND_STRING:
      case ND_SYMBOL:
         temp_reg = retain_reg();
         printf("lea %s, %s\n", id2reg64(temp_reg->id),
                gvar_node2reg(node, node->name));
         return temp_reg;

      case ND_DOT:
         lhs_reg = gen_register_leftval(node->lhs);
         if (node->type->offset > 0) {
            printf("add %s, %d\n", size2reg(8, lhs_reg), node->type->offset);
         }
         return lhs_reg;

      default:
         error("Error: NOT lvalue");
         return NO_REGISTER;
   }
}

void extend_al_ifneeded(Node *node, Register *reg) {
   if (type2size(node->type) > 1) {
      printf("movzx %s, al\n", node2reg(node, reg));
   } else {
      printf("mov %s, al\n", node2reg(node, reg));
   }
}

int save_reg() {
   int j;
   int stored_cnt = 0;
   for (j = 0; j < 6; j++) {
      if (reg_table[j] <= 0)
         continue;
      printf("push %s\n", registers64[j]);
      stored_cnt++;
   }
   return stored_cnt;
}

int restore_reg() {
   int j;
   int restored_cnt = 0;
   for (j = 0; j < 6; j++) {
      if (reg_table[j] <= 0)
         continue;
      printf("pop %s\n", registers64[j]);
      restored_cnt++;
   }
   return restored_cnt;
}

Register *gen_register_rightval(Node *node, int unused_eval) {
   Register *temp_reg;
   Register *lhs_reg;
   Register *rhs_reg;
   int j = 0;
   int cur_if_cnt;

   if (!node) {
      return NO_REGISTER;
   }

   // Write down line number
   if (node->pline >= 0) {
      // 1 means the file number
      printf(".loc 1 %d\n", node->pline);
   }
   switch (node->ty) {
      case ND_NUM:
         temp_reg = retain_reg();
         if (type2size(node->type) == 1) {
            // Delete previous value because of mov al, %ld will not delete all
            // value in rax
            printf("xor %s, %s\n", size2reg(8, temp_reg),
                   size2reg(8, temp_reg));
         }
         printf("mov %s, %ld\n", node2reg(node, temp_reg), node->num_val);
         return temp_reg;

      case ND_FLOAT:
         temp_reg = float_retain_reg();
         return temp_reg;

      case ND_STRING:
      case ND_SYMBOL:
         temp_reg = retain_reg();
         printf("lea %s, qword ptr %s[rip]\n", size2reg(8, temp_reg),
                node->name);
         return temp_reg;
         // return with toplevel char ptr.

      case ND_EXTERN_SYMBOL:
         // TODO should delete
         temp_reg = retain_reg();
         printf("mov %s, qword ptr [rip + %s@GOTPCREL]\n",
                size2reg(8, temp_reg), node->name);
         printf("mov %s, qword ptr [%s]\n", size2reg(8, temp_reg),
                size2reg(8, temp_reg));
         return temp_reg;

      case ND_IDENT:
         if (node->type->ty == TY_ARRAY) {
            return gen_register_leftval(node);
         } else {
            temp_reg = malloc(sizeof(Register));
            temp_reg->id = node->lvar_offset;
            temp_reg->kind = R_LVAR;
            temp_reg->name = NULL;
            temp_reg->size = type2size(node->type);
            return temp_reg;
         }

      case ND_GLOBAL_IDENT:
         if (node->type->ty == TY_ARRAY) {
            return gen_register_leftval(node);
         } else {
            temp_reg = malloc(sizeof(Register));
            temp_reg->id = 0;
            temp_reg->kind = R_GVAR;
            temp_reg->name = node->name;
            temp_reg->size = type2size(node->type);
            return temp_reg;
         }

      case ND_DOT:
         temp_reg = gen_register_leftval(node);
         if (node->type->ty != TY_ARRAY) {
            printf("mov %s, [%s]\n", node2reg(node, temp_reg),
                   id2reg64(temp_reg->id));
         }
         return temp_reg;

      case ND_ASSIGN:
         // This behaivour can be revised. like [rbp-8+2]
         if (node->lhs->ty == ND_IDENT || node->lhs->ty == ND_GLOBAL_IDENT) {
            lhs_reg = gen_register_rightval(node->lhs, 0);
            rhs_reg = gen_register_rightval(node->rhs, 0);
            secure_mutable(rhs_reg);
            if (node->lhs->type->ty == TY_FLOAT && node->rhs->type->ty == TY_FLOAT) {
               printf("movss %s, %s\n", node2reg(node->lhs, lhs_reg), node2reg(node->lhs, rhs_reg));
            } else {
               // TODO
               printf("mov %s, %s\n", node2reg(node->lhs, lhs_reg),
                     node2reg(node->lhs, rhs_reg));
            }
            release_reg(rhs_reg);
            if (unused_eval) {
               release_reg(lhs_reg);
            }
            return lhs_reg;
         } else {
            lhs_reg = gen_register_leftval(node->lhs);
            rhs_reg = gen_register_rightval(node->rhs, 0);
            // TODO
            secure_mutable(rhs_reg);
            printf("mov %s [%s], %s\n", node2specifier(node),
                   id2reg64(lhs_reg->id), node2reg(node->lhs, rhs_reg));
            release_reg(lhs_reg);
            if (unused_eval) {
               release_reg(rhs_reg);
            }
            return rhs_reg;
         }

      case ND_CAST:
         temp_reg = gen_register_rightval(node->lhs, 0);
         secure_mutable(temp_reg);
         if (type2size(node->type) > type2size(node->lhs->type)) {
            switch (type2size(node->lhs->type)) {
               case 1:
                  // TODO treat as unsigned char.
                  // for signed char, use movsx instead of.
                  printf("movzx %s, %s\n", node2reg(node, temp_reg),
                         id2reg8(temp_reg->id));
                  break;
               default:
                  break;
            }
         }
         return temp_reg;

      case ND_COMMA:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         release_reg(lhs_reg);
         return rhs_reg;

      case ND_ADD:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable(lhs_reg);
         if (node->lhs->type->ty == TY_FLOAT && node->rhs->type->ty == TY_FLOAT) {
            printf("addss %s, %s\n", node2reg(node, lhs_reg), node2reg(node, rhs_reg));
         } else {
            printf("add %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         }
         release_reg(rhs_reg);
         if (unused_eval) {
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            return lhs_reg;
         }

      case ND_SUB:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable(lhs_reg);
         if (node->lhs->type->ty == TY_FLOAT && node->rhs->type->ty == TY_FLOAT) {
            printf("subss %s, %s\n", node2reg(node, lhs_reg), node2reg(node, rhs_reg));
         } else {
            printf("sub %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         }
         release_reg(rhs_reg);
         if (unused_eval) {
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            return lhs_reg;
         }

      case ND_MULTIPLY_IMMUTABLE_VALUE:
         // after optimizing, this will be deleted.
         lhs_reg = gen_register_rightval(node->lhs, 0);
         secure_mutable(lhs_reg);
         printf("imul %s, %ld\n", node2reg(node, lhs_reg), node->num_val);
         return lhs_reg;

      case ND_MUL:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable(lhs_reg);
         if (node->lhs->type->ty == TY_FLOAT && node->rhs->type->ty == TY_FLOAT) {
            printf("mulss %s, %s\n", node2reg(node, lhs_reg), node2reg(node, rhs_reg));
         } else {
            printf("imul %s, %s\n", node2reg(node->lhs, lhs_reg),
                  node2reg(node->rhs, rhs_reg));
         }
         release_reg(rhs_reg);
         if (unused_eval) {
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            return lhs_reg;
         }

      case ND_DIV:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         // TODO should support char
         printf("mov %s, %s\n", _rax(node->lhs), node2reg(node->lhs, lhs_reg));
         puts("cqo");
         printf("idiv %s\n", node2reg(node->rhs, rhs_reg));
         secure_mutable(lhs_reg);
         printf("mov %s, %s\n", node2reg(node->lhs, lhs_reg), _rax(node->lhs));
         release_reg(rhs_reg);
         if (unused_eval) {
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            return lhs_reg;
         }

      case ND_MOD:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         // TODO should support char: idiv only support r32 or r64
         printf("mov %s, %s\n", _rax(node->lhs), node2reg(node->lhs, lhs_reg));
         puts("cqo");
         printf("idiv %s\n", node2reg(node->rhs, rhs_reg));
         secure_mutable(lhs_reg);
         printf("mov %s, %s\n", node2reg(node, lhs_reg), _rdx(node->lhs));
         release_reg(rhs_reg);
         if (unused_eval) {
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            return lhs_reg;
         }

      case ND_XOR:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable(lhs_reg);
         printf("xor %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         release_reg(rhs_reg);
         return lhs_reg;

      case ND_AND:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable(lhs_reg);
         printf("and %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         release_reg(rhs_reg);
         return lhs_reg;
         break;

      case ND_OR:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable(lhs_reg);
         printf("or %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         release_reg(rhs_reg);
         return lhs_reg;

      case ND_RSHIFT:
         // SHOULD be int
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         // FIXME: for signed int (Arthmetric)
         // mov rdi[8] -> rax
         printf("mov cl, %s\n", id2reg8(rhs_reg->id));
         release_reg(rhs_reg);
         secure_mutable(lhs_reg);
         printf("sar %s, cl\n", node2reg(node->lhs, lhs_reg));
         return lhs_reg;

      case ND_LSHIFT:
         // SHOULD be int
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         // FIXME: for signed int (Arthmetric)
         // mov rdi[8] -> rax
         // TODO Support minus
         printf("mov cl, %s\n", id2reg8(rhs_reg->id));
         release_reg(rhs_reg);
         secure_mutable(lhs_reg);
         printf("sal %s, cl\n", node2reg(node->lhs, lhs_reg));
         return lhs_reg;

      case ND_ISEQ:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         cmp_regs(node, lhs_reg, rhs_reg);
         // TODO Support minus
         puts("sete al");
         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_ISNOTEQ:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         cmp_regs(node, lhs_reg, rhs_reg);
         puts("setne al");
         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_GREATER:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable(rhs_reg);
         cmp_regs(node, lhs_reg, rhs_reg);
         puts("setg al");
         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_LESS:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);

         secure_mutable(rhs_reg);
         cmp_regs(node, lhs_reg, rhs_reg);
         // TODO: is "andb 1 %al" required?
         puts("setl al");
         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_ISMOREEQ:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         cmp_regs(node, lhs_reg, rhs_reg);
         puts("setge al");
         puts("and al, 1");
         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_ISLESSEQ:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         cmp_regs(node, lhs_reg, rhs_reg);
         puts("setle al");
         puts("and al, 1");
         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_RETURN:
         if (node->lhs) {
            lhs_reg = gen_register_rightval(node->lhs, 0);
            printf("mov %s, %s\n", _rax(node->lhs),
                   node2reg(node->lhs, lhs_reg));
            release_reg(lhs_reg);
         }
         puts("mov rsp, rbp");
         puts("pop rbp");
         puts("ret");
         return NO_REGISTER;

      case ND_ADDRESS:
         if (node->lhs->ty == ND_IDENT || node->lhs->ty == ND_GLOBAL_IDENT) {
            temp_reg = retain_reg();
            lhs_reg = gen_register_rightval(node->lhs, 0);
            printf("lea %s, %s\n", id2reg64(temp_reg->id),
                   node2reg(node, lhs_reg));
            release_reg(lhs_reg);
            return temp_reg;
         } else {
            return gen_register_leftval(node->lhs);
         }

      case ND_NEG:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         printf("neg %s\n", node2reg(node, lhs_reg));
         return lhs_reg;

      case ND_FPLUSPLUS:
         lhs_reg = gen_register_leftval(node->lhs);
         if (unused_eval) {
            secure_mutable(lhs_reg);
            printf("add %s [%s], %ld\n", node2specifier(node),
                   id2reg64(lhs_reg->id), node->num_val);
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            temp_reg = retain_reg();
            secure_mutable(lhs_reg);

            if (type2size(node->type) == 1) {
               printf("movzx %s, [%s]\n", size2reg(4, temp_reg),
                      id2reg64(lhs_reg->id));
            } else {
               printf("mov %s, [%s]\n", node2reg(node, temp_reg),
                      id2reg64(lhs_reg->id));
            }
            printf("add %s [%s], %ld\n", node2specifier(node),
                   id2reg64(lhs_reg->id), node->num_val);
            release_reg(lhs_reg);
            return temp_reg;
         }

      case ND_FSUBSUB:
         lhs_reg = gen_register_leftval(node->lhs);
         if (unused_eval) {
            secure_mutable(lhs_reg);
            printf("sub %s [%s], %ld\n", node2specifier(node),
                   id2reg64(lhs_reg->id), node->num_val);
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            temp_reg = retain_reg();
            secure_mutable(lhs_reg);

            printf("mov %s, [%s]\n", node2reg(node, temp_reg),
                   id2reg64(lhs_reg->id));
            printf("sub %s [%s], %ld\n", node2specifier(node),
                   id2reg64(lhs_reg->id), node->num_val);
            release_reg(lhs_reg);
            return temp_reg;
         }

      case ND_APOS:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         printf("cmp %s, 0\n", node2reg(node->lhs, lhs_reg));
         puts("sete al");
         release_reg(lhs_reg);
         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_DEREF:
         lhs_reg = gen_register_rightval(node->lhs, 0);

         // this is because we cannot [[lhs_reg]] (float deref)
         secure_mutable(lhs_reg);
         if (type2size(node->type) == 1) {
            // when reading char, we should read just 1 byte
            printf("movzx %s, byte ptr [%s]\n", size2reg(4, lhs_reg),
                   size2reg(8, lhs_reg));
         } else if (node->type->ty != TY_ARRAY) {
            printf("mov %s, [%s]\n", node2reg(node, lhs_reg),
                   size2reg(8, lhs_reg));
         }
         return lhs_reg;

      case ND_FDEF:
         printf(".type %s,@function\n", node->gen_name);
         printf(".global %s\n", node->gen_name); // to visible for ld
         printf("%s:\n", node->gen_name);
         puts("push rbp");
         puts("mov rbp, rsp");
         printf("sub rsp, %d\n", *node->env->rsp_offset_max);
         for (j = 0; j < node->argc; j++) {
            temp_reg = gen_register_rightval(node->args[j], 0);
            // TODO : not to use eax, so on
            printf("mov %s, %s\n", size2reg(8, temp_reg), arg_registers[j]);
         }
         if (node->is_omiited) {
            for (j = 0; j < 6; j++) {
               printf("mov [rbp-%d], %s\n",
                      node->is_omiited->lvar_offset - j * 8, arg_registers[j]);
            }
            for (j=0;j<8;j++) {
               printf("movaps [rbp-%d], xmm%d\n", node->is_omiited->lvar_offset - 8 * 6 - j *16, j);
            }
         }
         for (j = 0; j < node->code->len; j++) {
            // read inside functions.
            gen_register_rightval((Node *)node->code->data[j], 1);
         }
         if (node->type->ret->ty == TY_VOID) {
            puts("mov rsp, rbp");
            puts("pop rbp");
            puts("ret");
         }

         // Release All Registers.
         release_all_reg();
         return NO_REGISTER;

      case ND_BLOCK:
         for (j = 0; j < node->code->len && node->code->data[j]; j++) {
            gen_register_rightval((Node *)node->code->data[j], 1);
         }
         return NO_REGISTER;

      case ND_EXPRESSION_BLOCK:
         puts("mov rbp, rsp");
         printf("sub rsp, %d\n", *node->env->rsp_offset_max);
         puts("#test begin expansion");
         for (j = 0; j < node->argc; j++) {
            temp_reg = gen_register_rightval(node->args[j], 0);
            gen_register_rightval(node->args[j], 1);
         }
         puts("#test end expansion of args");
         for (j = 0; j < node->code->len && node->code->data[j]; j++) {
            gen_register_rightval((Node *)node->code->data[j], 1);
         }
         puts("mov rsp, rbp");
         puts("#test end expansion ");
         return NO_REGISTER;

      case ND_FUNC:
         for (j = 0; j < node->argc; j++) {
            temp_reg = gen_register_rightval(node->args[j], 0);
            // TODO Dirty: Should implement 642node
            secure_mutable(temp_reg);
            printf("push %s\n", id2reg64(temp_reg->id));
            release_reg(temp_reg);
         }
         for (j = node->argc - 1; j >= 0; j--) {
            // because of function call will break these registers
            printf("pop %s\n", arg_registers[j]);
         }

         save_reg();
         // FIXME: alignment should be 64-bit
         puts("mov al, 0");
         if (node->gen_name) { // ND_GLOBAL_IDENT, called from local vars.
            printf("call %s\n",
                   node->gen_name); // rax should be aligned with the size
         } else if (node->type->context->is_previous_class) {
            char *buf = malloc(sizeof(char) * 256);
            snprintf(buf, 255, "%s::%s", node->type->context->is_previous_class,
                     node->type->context->method_name);
            printf("call %s\n", mangle_func_name(buf));
         } else {
            temp_reg = gen_register_rightval(node->lhs, 0);
            secure_mutable(temp_reg);
            printf("call %s\n", size2reg(8, temp_reg));
            release_reg(temp_reg);
         }
         restore_reg();

         if (node->type->ty == TY_VOID || unused_eval == 1) {
            return NO_REGISTER;
         } else {
            temp_reg = retain_reg();
            printf("mov %s, %s\n", node2reg(node, temp_reg), _rax(node));
            return temp_reg;
         }

      case ND_VAARG:
         // SEE in AMD64 ABI Draft 0.99.7 p.53 (uglibc.org )
         lhs_reg = gen_register_leftval(node->lhs); // meaning ap
         temp_reg = retain_reg();

         // gp_offset
         printf("mov eax, [%s]\n", id2reg64(lhs_reg->id)); // get gp_offset
         printf("mov edx, eax\n");
         printf("add rdx, 8\n");
         printf("add rax, [%s+16]\n", id2reg64(lhs_reg->id));
         printf("mov [%s], edx\n", id2reg64(lhs_reg->id));
         // only supported register
         printf("mov %s, [rax]\n",
                node2reg(node, temp_reg)); // 1->reg_saved_area
         release_reg(lhs_reg);

         return temp_reg;

      case ND_VASTART:
         lhs_reg = gen_register_leftval(node->lhs); // meaning ap's address
         // node->num_val means the first undefined argument No.
         // from lval_offset:
         // rbp-80 : r0
         // rbp-72 : r1
         // ...
         // rbp-56: r6
         printf("mov dword ptr [%s], %ld\n", id2reg64(lhs_reg->id),
                node->num_val * 8);
         printf("mov dword ptr [%s+4], 304\n", id2reg64(lhs_reg->id));
         printf("lea rax, [rbp-%d]\n", node->rhs->lvar_offset);
         printf("mov qword ptr [%s+16], rax\n", id2reg64(lhs_reg->id));
         /*
         // set reg_save_area
         printf("mov [%s], rax\n", node2reg(node->lhs, lhs_reg));
         printf("lea rax, [rbp]\n");
         // set overflow_arg_area
         printf("mov [%s+4], rax\n", node2reg(node->lhs, lhs_reg));
         */
         return NO_REGISTER;

      case ND_VAEND:
         return NO_REGISTER;

      case ND_GOTO:
         printf("jmp %s\n", node->name);
         return NO_REGISTER;

      case ND_LOR:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         printf("cmp %s, 0\n", node2reg(node->lhs, lhs_reg));
         release_reg(lhs_reg);
         cur_if_cnt = if_cnt++;
         printf("jne .Lorend%d\n", cur_if_cnt);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         printf("cmp %s, 0\n", node2reg(node->rhs, rhs_reg));
         release_reg(rhs_reg);
         printf(".Lorend%d:\n", cur_if_cnt);
         puts("setne al");
         temp_reg = retain_reg();
         if (type2size(node->type) == 1) {
            printf("mov %s, al\n", node2reg(node, temp_reg));
         } else {
            printf("movzx %s, al\n", node2reg(node, temp_reg));
         }
         return temp_reg;

      case ND_LAND:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         printf("cmp %s, 0\n", node2reg(node->lhs, lhs_reg));
         release_reg(lhs_reg);
         cur_if_cnt = if_cnt++;
         printf("je .Lorend%d\n", cur_if_cnt);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         printf("cmp %s, 0\n", node2reg(node->rhs, rhs_reg));
         release_reg(rhs_reg);
         printf(".Lorend%d:\n", cur_if_cnt);
         puts("setne al");
         temp_reg = retain_reg();
         if (type2size(node->type) == 1) {
            printf("mov %s, al\n", node2reg(node, temp_reg));
         } else {
            printf("movzx %s, al\n", node2reg(node, temp_reg));
         }
         return temp_reg;

      case ND_IF:
         temp_reg = gen_register_rightval(node->args[0], 0);
         printf("cmp %s, 0\n", node2reg(node->args[0], temp_reg));
         release_reg(temp_reg);
         cur_if_cnt = if_cnt++;
         printf("je .Lendif%d\n", cur_if_cnt);
         gen_register_rightval(node->lhs, 1);
         if (node->rhs) {
            printf("jmp .Lelseend%d\n", cur_if_cnt);
         }
         printf(".Lendif%d:\n", cur_if_cnt);
         if (node->rhs) {
            // There are else
            gen_register_rightval(node->rhs, 1);
            printf(".Lelseend%d:\n", cur_if_cnt);
         }
         return NO_REGISTER;

      case ND_SWITCH:
         // TODO: quite dirty
         cur_if_cnt = for_while_cnt++;
         int prev_env_for_while_switch = env_for_while_switch;
         env_for_while_switch = cur_if_cnt;

         lhs_reg = gen_register_rightval(node->lhs, 0);
         // find CASE Labels and lookup into args[0]->code->data
         for (j = 0; node->rhs->code->data[j]; j++) {
            Node *curnode = (Node *)node->rhs->code->data[j];
            if (curnode->ty == ND_CASE) {
               char *input = malloc(sizeof(char) * 256);
               snprintf(input, 255, ".L%dC%d", cur_if_cnt, j);
               curnode->name = input; // assign unique ID

               if (type2size(node->lhs->type) !=
                   type2size(curnode->lhs->type)) {
                  curnode->lhs = new_node(ND_CAST, curnode->lhs, NULL);
                  curnode->lhs->type = node->lhs->type;
               }
               temp_reg = gen_register_rightval(curnode->lhs, 0);
               printf("cmp %s, %s\n", node2reg(node->lhs, lhs_reg),
                      node2reg(curnode->lhs, temp_reg));
               release_reg(temp_reg);
               printf("je %s\n", input);
            }
            if (curnode->ty == ND_DEFAULT) {
               char *input = malloc(sizeof(char) * 256);
               snprintf(input, 255, ".L%dC%d", cur_if_cnt, j);
               curnode->name = input;
               printf("jmp %s\n", input);
            }
         }
         release_reg(lhs_reg);
         // content
         gen_register_rightval(node->rhs, 1);
         printf(".Lend%d:\n", cur_if_cnt);
         env_for_while_switch = prev_env_for_while_switch;
         return NO_REGISTER;

      case ND_CASE:
      case ND_DEFAULT:
         // just an def. of goto
         // saved with input
         if (!node->name) {
            error("Error: case statement without switch\n");
         }
         printf("%s:\n", node->name);
         return NO_REGISTER;

      case ND_FOR:
      case ND_WHILE:
      case ND_DOWHILE:
         cur_if_cnt = for_while_cnt++;
         int prev_env_for_while = env_for_while;
         prev_env_for_while_switch = env_for_while_switch;
         env_for_while = cur_if_cnt;
         env_for_while_switch = cur_if_cnt;

         // TODO dirty: compare node->ty twice
         switch (node->ty) {
            case ND_FOR:
               gen_register_rightval(node->conds[0], 1);
               printf("jmp .Lcondition%d\n", cur_if_cnt);
               printf(".Lbeginwork%d:\n", cur_if_cnt);
               gen_register_rightval(node->rhs, 1);
               printf(".Lbegin%d:\n", cur_if_cnt);
               gen_register_rightval(node->conds[2], 1);
               printf(".Lcondition%d:\n", cur_if_cnt);
               temp_reg = gen_register_rightval(node->conds[1], 0);

               printf("cmp %s, 0\n", node2reg(node->conds[1], temp_reg));
               release_reg(temp_reg);
               printf("jne .Lbeginwork%d\n", cur_if_cnt);
               printf(".Lend%d:\n", cur_if_cnt);
               break;

            case ND_WHILE:
               printf(".Lbegin%d:\n", cur_if_cnt);
               lhs_reg = gen_register_rightval(node->lhs, 0);
               printf("cmp %s, 0\n", node2reg(node->lhs, lhs_reg));
               release_reg(lhs_reg);
               printf("je .Lend%d\n", cur_if_cnt);
               gen_register_rightval(node->rhs, 1);
               printf("jmp .Lbegin%d\n", cur_if_cnt);
               printf(".Lend%d:\n", cur_if_cnt);
               break;

            case ND_DOWHILE:
               printf(".Lbegin%d:\n", cur_if_cnt);
               gen_register_rightval(node->rhs, 1);
               lhs_reg = gen_register_rightval(node->lhs, 0);
               printf("cmp %s, 0\n", node2reg(node->lhs, lhs_reg));
               release_reg(lhs_reg);
               printf("jne .Lbegin%d\n", cur_if_cnt);
               printf(".Lend%d:\n", cur_if_cnt);
               break;
            default:
               break;
         }

         env_for_while = prev_env_for_while;
         env_for_while_switch = prev_env_for_while_switch;
         return NO_REGISTER;

      case ND_BREAK:
         printf("jmp .Lend%d #break\n", env_for_while_switch);
         return NO_REGISTER;

      case ND_CONTINUE:
         printf("jmp .Lbegin%d\n", env_for_while);
         return NO_REGISTER;

      default:
         fprintf(stderr, "Error: Incorrect Registers %d.\n", node->ty);
         exit(1);
   }
   return NO_REGISTER;
}

void gen_register_top() {
   init_reg_table();
   init_reg_registers();
   for (int j = 0; j < globalcode->len; j++) {
      gen_register_rightval((Node *)globalcode->data[j], 1);
   }
}


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
         // TODO should be fixed
         node = new_assign_node(node, new_node_with_cast(tp, node, assign()));
      } else {
         node = new_assign_node(node, assign());
      }
   }
   return node;
}

Type *read_type(Type *type, char **input, Map *local_typedb) {
   int ptr_cnt = 0;
   int i = 0;
   Type *concrete_type;

   // Variable Definition.
   // consume is pointer or not
   while (consume_node('*')) {
      ptr_cnt++;
   }
   // There are input: there are ident names
   if (input) {
      // When there are its name or not
      *input = NULL;
      if (confirm_node(TK_IDENT)) {
         *input = tokens->data[pos]->input;
      } else if (consume_node('(')) {
         // functional pointer. declarator
         concrete_type = read_type(NULL, input, local_typedb);
         Type *base_type = concrete_type;
         while (base_type->ptrof != NULL) {
            base_type = base_type->ptrof;
         }
         type = concrete_type;
         expect_node(')');
      }
   } else {
      input = &tokens->data[pos]->input;
   }
   consume_node(TK_IDENT);

   for (i = 0; i < ptr_cnt; i++) {
      type = type_pointer(type);
   }
   // array
   while (1) {
      if (consume_node('[')) {
         Type *base_type = type;
         type = new_type();
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
            cur_ptr->ptrof = new_type();
            cur_ptr->ptrof->ty = TY_ARRAY;
            cur_ptr->ptrof->array_size = (int)tokens->data[pos]->num_val;
            cur_ptr->ptrof->ptrof = base_type;
            expect_node(TK_NUM);
            expect_node(']');
            cur_ptr = cur_ptr->ptrof;
         }
      } else if (consume_node('(')) {
         // Function call.
         Type *concrete_type = new_type();
         concrete_type->ty = TY_FUNC;
         concrete_type->ret = type;
         concrete_type->argc = 0;
         concrete_type->is_omiited = 0;

         // Propagate is_static func or not
         type->is_static = concrete_type->is_static;

         // follow concrete_type if real type !is not supported yet!
         // to set...
         // BEFORE: TY_PTR -> TY_PTR -> TY_PTR -> TY_INT
         // AFTER: TY_FUNC
         type = concrete_type;

         // treat as function.
         for (concrete_type->argc = 0;
              concrete_type->argc <= 6 && !consume_node(')');) {
            if (consume_node(TK_OMIITED)) {
               type->is_omiited = 1;
               expect_node(')');
               break;
            }
            char *buf;
            concrete_type->args[concrete_type->argc] =
                read_fundamental_type(local_typedb);
            concrete_type->args[concrete_type->argc] = read_type(
                concrete_type->args[concrete_type->argc], &buf, local_typedb);
            // Save its variable name, if any.
            concrete_type->args[concrete_type->argc++]->name = buf;
            consume_node(',');
         }
      } else {
         break;
      }
   }
   return type;
}

Type *read_type_all(char **input) {
   Type *type = read_fundamental_type(NULL);
   type = read_type(type, input, NULL);
   return type;
}

Node *stmt() {
   Node *node = NULL;
   int pline = tokens->data[pos]->pline;
   if (confirm_type()) {
      char *input = NULL;
      // FIXME
      Type *fundamental_type = read_fundamental_type(current_local_typedb);
      Type *type;
      do {
         type = read_type(fundamental_type, &input, NULL);
         node = new_ident_node_with_new_variable(input, type);
         // if there is int a =1;
         if (consume_node('=')) {
            if (consume_node('{')) {
            } else {
               node = new_node('=', node, node_mathexpr());
            }
         }
         input = NULL;
      } while (consume_node(','));
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
   node->pline = pline;
   return node;
}

Node *node_if() {
   Node *node = new_node(ND_IF, NULL, NULL);
   node->argc = 1;
   node->args[0] = assign();
   node->args[1] = NULL;
   node->pline = tokens->data[pos - 1]->pline;
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
         vec_push(args, (Token *)new_block);
         continue;
      }

      if (consume_node(TK_IF)) {
         vec_push(args, (Token *)node_if());
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
         vec_push(args, (Token *)while_node);
         continue;
      }
      if (consume_node(TK_DO)) {
         Node *do_node = new_node(ND_DOWHILE, NULL, NULL);
         do_node->rhs = new_block_node(env);
         program(do_node->rhs);
         expect_node(TK_WHILE);
         do_node->lhs = node_mathexpr();
         expect_node(';');
         vec_push(args, (Token *)do_node);
         continue;
      }
      if (consume_node(TK_FOR)) {
         Node *for_node = new_node(ND_FOR, NULL, NULL);
         expect_node('(');
         // TODO: should be splited between definition and expression
         for_node->conds[0] = stmt();
         for_node->rhs = new_block_node(env);
         // expect_node(';');
         // TODO: to allow without lines
         if (!consume_node(';')) {
            for_node->conds[1] = assign();
            expect_node(';');
         }
         if (!consume_node(')')) {
            for_node->conds[2] = assign();
            expect_node(')');
         }
         program(for_node->rhs);
         vec_push(args, (Token *)for_node);
         continue;
      }

      if (consume_node(TK_CASE)) {
         vec_push(args, (Token *)new_node(ND_CASE, node_term(), NULL));
         expect_node(':');
         continue;
      }
      if (consume_node(TK_DEFAULT)) {
         vec_push(args, (Token *)new_node(ND_DEFAULT, NULL, NULL));
         expect_node(':');
         continue;
      }

      if (consume_node(TK_SWITCH)) {
         expect_node('(');
         Node *switch_node = new_node(ND_SWITCH, node_mathexpr(), NULL);
         expect_node(')');
         switch_node->rhs = new_block_node(env);
         program(switch_node->rhs);
         vec_push(args, (Token *)switch_node);
         continue;
      }
      vec_push(args, (Token *)stmt());
   }
   vec_push(args, NULL);

   // 'consumed }'
   env = prev_env;
}

// 0: neither 1:TK_TYPE 2:TK_IDENT

Type *duplicate_type(Type *old_type) {
   Type *type = new_type();
   type = copy_type(old_type, type);
   return type;
}

Node *copy_node(Node *old_node, Node *node) {
   // only supported primitive node
   node->ty = old_node->ty;
   node->lhs = old_node->lhs;
   node->rhs = old_node->rhs;
   node->conds[0] = old_node->conds[0];
   node->conds[1] = old_node->conds[1];
   node->conds[2] = old_node->conds[2];

   node->code = old_node->code;
   node->argc = old_node->argc;
   node->num_val = old_node->num_val;
   node->float_val = old_node->float_val;
   node->name = old_node->name;
   node->gen_name = old_node->gen_name;
   node->env = old_node->env;
   node->type = duplicate_type(old_node->type);
   node->lvar_offset = old_node->lvar_offset;
   node->is_omiited = old_node->is_omiited;
   node->is_static = old_node->is_static;
   node->is_recursive = old_node->is_static;
   node->args[0] = old_node->args[0];
   node->args[1] = old_node->args[1];
   node->args[2] = old_node->args[2];
   node->args[3] = old_node->args[3];
   node->args[4] = old_node->args[4];
   node->args[5] = old_node->args[5];
   node->pline = -1; // TODO for more advenced text
   node->funcdef = old_node->funcdef;
   return node;
}

Node *duplicate_node(Node *old_node) {
   Node *node = new_node(0, NULL, NULL);
   node = copy_node(old_node, node);
   return node;
}

Type *copy_type(Type *old_type, Type *type) {
   if (!old_type) {
      return NULL;
   }
   type->ty = old_type->ty;
   type->structure = old_type->structure;
   type->array_size = old_type->array_size;
   type->ptrof = old_type->ptrof;
   type->offset = old_type->offset;
   type->is_const = old_type->is_const;
   type->is_static = old_type->is_static;
   type->name = old_type->name;
   type->ret = old_type->ret;
   type->template_name = old_type->template_name;
   return type;
}

Type *find_typed_db(char *input, Map *db) {
   if (!input) {
      fprintf(stderr, "Error: find_typed_db with null input\n");
      exit(1);
   }
   for (int j = 0; j < db->keys->len; j++) {
      // for struct
      if (strcmp(input, (char *)db->keys->data[j]) == 0) {
         // copy type
         Type *old_type = (Type *)db->vals->data[j];
         return duplicate_type(old_type);
      }
   }
   return NULL;
}

Type *read_fundamental_type(Map *local_typedb) {
   int is_const = 0;
   int is_static = 0;
   char *template_typename = NULL;
   Type *type = NULL;

   while (1) {
      if (tokens->data[pos]->ty == TK_STATIC) {
         is_static = 1;
         expect_node(TK_STATIC);
      } else if (tokens->data[pos]->ty == TK_CONST) {
         is_const = 1;
         expect_node(TK_CONST);
      } else if (consume_node(TK_TEMPLATE)) {
         expect_node('<');
         while (1) {
            expect_node(TK_TYPENAME);
            template_typename = expect_ident();
            type = new_type();
            type->ty = TY_TEMPLATE;
            type->template_name = template_typename;
            map_put(local_typedb, template_typename, type);
            if (!consume_node(',')) {
               break;
            }
         }
         expect_node('>');
         break;
      } else {
         break;
      }
   }

   if (tokens->data[pos]->ty == TK_ENUM) {
      // treat as anonymous enum
      expect_node(TK_ENUM);
      define_enum(0);
      type = find_typed_db("int", typedb);
   } else if (consume_node(TK_STRUCT)) {
      type = find_typed_db(expect_ident(), struct_typedb);
   } else {
      char *ident = expect_ident();
      if (local_typedb && local_typedb->keys->len > 0) {
         type = find_typed_db(ident, local_typedb);
         if (!type) {
            type = find_typed_db(ident, typedb);
         }
      } else {
         type = find_typed_db(ident, typedb);
      }
   }
   if (type) {
      type->is_const = is_const;
      type->is_static = is_static;
   }
   return type;
}

int split_type_caller() {
   int tos = pos; // for skipping tk_static
   // static may be ident or func
   if (tokens->data[tos]->ty == TK_STATIC) {
      tos++;
   }
   if (tokens->data[tos]->ty == TK_TEMPLATE) {
      return 3;
   }
   if (tokens->data[tos]->ty != TK_IDENT) {
      return 0;
   }
   for (int j = 0; j < typedb->keys->len; j++) {
      // for struct
      if (strcmp(tokens->data[tos]->input, (char *)typedb->keys->data[j]) ==
          0) {
         return typedb->vals->data[j]->ty;
      }
   }
   for (int j = 0; current_local_typedb && j < current_local_typedb->keys->len;
        j++) {
      // for template
      if (strcmp(tokens->data[tos]->input,
                 (char *)current_local_typedb->keys->data[j]) == 0) {
         return 3;
      }
   }
   return 2; // IDENT
}

int confirm_type() {
   if (split_type_caller() > 2) {
      return 1;
   }
   return 0;
}
int confirm_ident() {
   if (split_type_caller() == 2) {
      return 1;
   }
   return 0;
}

int consume_ident() {
   if (split_type_caller() == 2) {
      pos++;
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
   consume_ident(); // for ease
   expect_node('{');
   Type *enumtype = new_type();
   enumtype->ty = TY_INT;
   enumtype->offset = 4;
   int cnt = 0;
   while (!consume_node('}')) {
      char *itemname = expect_ident();
      Node *itemnode = NULL;
      if (consume_node('=')) {
         itemnode = new_num_node_from_token(tokens->data[pos]);
         cnt = tokens->data[pos]->num_val;
         expect_node(TK_NUM);
      } else {
         itemnode = new_num_node(cnt);
      }
      cnt++;
      map_put(consts, itemname, itemnode);
      if (!consume_node(',')) {
         expect_node('}');
         break;
      }
   }
   // to support anonymous enum
   if (assign_name && confirm_ident()) {
      char *name = expect_ident();
      map_put(typedb, name, enumtype);
   }
}

void new_fdef(char *name, Type *type, Map *local_typedb) {
   Node *newfunc;
   // Type *previoustype;
   int pline = tokens->data[pos]->pline;

   // This is type->ret->is_static because read_fundamental_type() calls static
   // but it will be resulted in type->ret
   newfunc = new_fdef_node(name, type, type->ret->is_static);
   newfunc->type->local_typedb = local_typedb;
   newfunc->pline = pline;
   // Function definition because toplevel func call

   // On CPP: Read from Previous Definition on Struct
   /*
   if ((lang & 1) && strstr(name, "::")) {
      previoustype = find_typed_db(typedb, subname);
   }
   */
   // FIXME

   // edit for instance function.
   if ((lang & 1) && strstr(name, "::") && type->ty == TY_FUNC &&
       type->ret->is_static == 0) {
      char *class_name = strtok(strdup(name), "::");
      Type *thistype = new_type();
      thistype->ptrof = find_typed_db(class_name, typedb);
      thistype->ty = TY_PTR;
      thistype->name = "this";
      type->args[5] = type->args[4];
      type->args[4] = type->args[3];
      type->args[3] = type->args[2];
      type->args[2] = type->args[1];
      type->args[1] = type->args[0];
      type->args[0] = thistype;
      type->argc++;

      type->context->is_previous_class = class_name;
      type->context->method_name = name;
   }
   // TODO env should be treated as cooler bc. of splitted namespaces
   Env *prev_env = env;
   env = newfunc->env;
   if (type->is_omiited > 0) {
      Type *saved_var_type = new_type();
      saved_var_type->ty = TY_ARRAY;
      saved_var_type->ptrof = find_typed_db("long", typedb);
      saved_var_type->array_size = 30; // for rsp
      newfunc->is_omiited =
          new_ident_node_with_new_variable("_saved_var", saved_var_type);
      omiited_argc = newfunc->argc;
   }
   newfunc->argc = type->argc;
   int i;
   for (i = 0; i < newfunc->argc; i++) {
      newfunc->args[i] =
          new_ident_node_with_new_variable(type->args[i]->name, type->args[i]);
   }
   env = prev_env;
   // to support prototype def.
   if (confirm_node('{')) {
      current_local_typedb = local_typedb;
      program(newfunc);
      current_local_typedb = NULL;
      newfunc->is_recursive = is_recursive(newfunc, name);
      if (local_typedb->keys->len <= 0) {
         // There are no typedb: template
         vec_push(globalcode, (Token *)newfunc);
      }
   } else {
      expect_node(';');
   }
}

void toplevel() {
   strcpy(arg_registers[0], "rdi");
   strcpy(arg_registers[1], "rsi");
   strcpy(arg_registers[2], "rdx");
   strcpy(arg_registers[3], "rcx");
   strcpy(arg_registers[4], "r8");
   strcpy(arg_registers[5], "r9");

   // consume_node('{')
   // idents = new_map...
   // stmt....
   // consume_node('}')
   global_vars = new_map();
   funcdefs = new_map();
   funcdefs_generated_template = new_map();
   consts = new_map();
   strs = new_vector();
   globalcode = new_vector();
   env = NULL;

   while (!consume_node(TK_EOF)) {
      if (consume_node(TK_EXTERN)) {
         char *name = NULL;
         Type *type = read_type_all(&name);
         type->offset = -1; // TODO externed
         map_put(global_vars, name, type);
         expect_node(';');
         continue;
      }
      // definition of class
      if ((lang & 1) && consume_node(TK_CLASS)) {
         Type *structuretype = new_type();
         structuretype->structure = new_map();
         structuretype->ty = TY_STRUCT;
         structuretype->ptrof = NULL;
         int offset = 0;
         int size = 0;
         char *structurename = expect_ident();
         structuretype->name = structurename;
         expect_node('{');
         MemberAccess memaccess;
         memaccess = PRIVATE; // on Struct, it is public
         while (!consume_node('}')) {
            if (consume_node(TK_PUBLIC)) {
               memaccess = PUBLIC;
               expect_node(':');
               continue;
            }
            if (consume_node(TK_PRIVATE)) {
               memaccess = PRIVATE;
               expect_node(':');
               continue;
            }
            char *name = NULL;
            Type *type = read_type_all(&name);
            size = type2size3(type);

            // TODO it should be integrated into new_fdef_node()'s "this"
            if ((lang & 1) && strstr(name, "::") && type->ty == TY_FUNC &&
                type->is_static == 0) {
               type->context->is_previous_class = structurename;
            }
            if (size > 0 && (offset % size != 0)) {
               offset += (size - offset % size);
            }
            // all type should aligned with proper value.
            // TODO assumption there are NO bytes over 8 bytes.
            type->offset = offset;
            offset += cnt_size(type);
            expect_node(';');
            type->memaccess = memaccess;
            map_put(structuretype->structure, name, type);
         }
         structuretype->offset = offset;
         expect_node(';');
         map_put(typedb, structurename, structuretype);
         continue;
      }

      if (consume_node(TK_TYPEDEF)) {
         if (consume_node(TK_ENUM)) {
            // not anonymous enum
            define_enum(1);
            expect_node(';');
            continue;
         }
         Type *structuretype = new_type();
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
            Type *type = read_type_all(&name);
            size = type2size3(type);
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
         structuretype->name = name;
         expect_node(';');
         map_put(typedb, name, structuretype);
         continue;
      }

      if (consume_node(TK_ENUM)) {
         consume_ident(); // for ease
         expect_node('{');
         char *name = expect_ident();
         Type *structuretype = new_type();
         expect_node(';');
         map_put(typedb, name, structuretype);
      }

      if (confirm_type()) {
         char *name = NULL;
         Map *local_typedb = new_map();
         Type *type = read_fundamental_type(local_typedb);
         type = read_type(type, &name, local_typedb);
         if (type->ty == TY_FUNC) {
            new_fdef(name, type, local_typedb);
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
         Node *newblocknode = new_block_node(NULL);
         program(newblocknode);
         vec_push(globalcode, (Token *)newblocknode);
         continue;
      }
      vec_push(globalcode, (Token *)stmt());
   }
   vec_push(globalcode, (Token *)new_block_node(NULL));
}

Type *generate_class_template(Type *type, Type *new_type) {
   type = duplicate_type(type);
   int j;

   if (type && type->ty == TY_TEMPLATE) {
      type = copy_type(new_type, type);
   }
   if (type->ptrof) {
      type = generate_class_template(type->ptrof, new_type);
   }
   if (type->ret) {
      type = generate_class_template(type->ret, new_type);
   }
   if (type->argc > 0) {
      for (j = 0; j < type->argc; j++) {
         if (type->args[j]) {
            type->args[j] = generate_class_template(type->args[j], new_type);
         }
      }
   }
   if (type->structure) {
      for (j = 0; j < type->structure->keys->len; j++) {
         if (type->structure->vals->data[j]) {
            type->structure->vals->data[j] = (Token *)generate_class_template(
                (Type *)type->structure->vals->data[j], new_type);
         }
      }
   }
   return type;
}

Node *generate_template(Node *node, Map *template_type_db) {
   node = duplicate_node(node);
   int j;
   Node *code_node;
   Type *new_type;
   if (node->type && node->type->ty == TY_TEMPLATE) {
      new_type = find_typed_db(node->type->template_name, template_type_db);
      if (!new_type) {
         fprintf(stderr, "Error: Incorrect Template type: %s\n",
                 node->type->template_name);
         exit(1);
      }
      node->type = copy_type(new_type, new_type);
   }
   if (node->lhs) {
      node->lhs = generate_template(node->lhs, template_type_db);
   }
   if (node->rhs) {
      node->rhs = generate_template(node->rhs, template_type_db);
   }
   if (node->code) {
      Vector *vec = new_vector();
      for (j = 0; j < node->code->len && node->code->data[j]; j++) {
         code_node =
             generate_template((Node *)node->code->data[j], template_type_db);
         vec_push(vec, (Token *)code_node);
      }
      node->code = vec;
   }
   if (node->argc > 0) {
      for (j = 0; j < node->argc; j++) {
         if (node->args[j]) {
            node->args[j] = generate_template(node->args[j], template_type_db);
         }
      }
   }

   for (j = 0; j < 3; j++) {
      if (node->conds[j]) {
         node->conds[j] = generate_template(node->conds[j], template_type_db);
      }
   }
   return node;
}

int is_recursive(Node *node, char *name) {
   int j;
   if (!node) {
      return 0;
   }
   if (node->ty == ND_FUNC) {
      if (node->name && strcmp(node->name, name) == 0) {
         return 1;
      }
   }

   if (is_recursive(node->lhs, name)) {
      return 1;
   }
   if (is_recursive(node->rhs, name)) {
      return 1;
   }

   if (node->code) {
      for (j = 0; j < node->code->len && node->code->data[j]; j++) {
         if (is_recursive((Node *)node->code->data[j], name)) {
            return 1;
         }
      }
   }

   for (j = 0; j < node->argc; j++) {
      if (is_recursive(node->args[j], name)) {
         return 1;
      }
   }

   for (j = 0; j < 3; j++) {
      if (is_recursive(node->conds[j], name)) {
         return 1;
      }
   }
   return 0;
}

void test_map() {
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

void init_typedb() {
   enum_typedb = new_map();
   struct_typedb = new_map();
   typedb = new_map();
   current_local_typedb = NULL; // Initiazlied on function.

   Type *typeint = new_type();
   typeint->ty = TY_INT;
   typeint->ptrof = NULL;
   map_put(typedb, "int", typeint);

   Type *typechar = new_type();
   typechar->ty = TY_CHAR;
   typechar->ptrof = NULL;
   map_put(typedb, "char", typechar);

   Type *typelong = new_type();
   typelong->ty = TY_LONG;
   typelong->ptrof = NULL;
   map_put(typedb, "long", typelong);

   Type *typevoid = new_type();
   typevoid->ty = TY_VOID;
   typevoid->ptrof = NULL;
   map_put(typedb, "void", typevoid);

   typevoid = new_type();
   typevoid->ty = TY_STRUCT;
   typevoid->ptrof = NULL;
   typevoid->offset = 8;
   typevoid->structure = new_map();
   map_put(typedb, "FILE", typevoid);

   Type *va_listtype = new_type();
   va_listtype->structure = new_map();
   va_listtype->ty = TY_STRUCT;
   va_listtype->ptrof = NULL;

   Type *type;
   type = find_typed_db("int", typedb);
   type->offset = 0;
   map_put(va_listtype->structure, "gp_offset", type);
   type = find_typed_db("int", typedb);
   type->offset = 4;
   map_put(va_listtype->structure, "fp_offset", type);

   type = new_type();
   type->ty = TY_PTR;
   type->ptrof = new_type();
   type->ptrof->ty = TY_VOID;
   type->ptrof->ptrof = NULL;
   type->offset = 8;

   map_put(va_listtype->structure, "overflow_arg_area", type);
   type = duplicate_type(type);
   type->offset = 16;
   map_put(va_listtype->structure, "reg_save_area", type);
   va_listtype->offset = 4 + 4 + 8 + 8;
   map_put(typedb, "va_list", va_listtype);

   Type *typedou = new_type();
   typedou->ty = TY_FLOAT;
   typedou->ptrof = NULL;
   typedou->offset = 8;
   map_put(typedb, "float", typevoid);
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

Node *analyzing(Node *node) {
   int j;
   Env *prev_env = env;
   if (node->ty == ND_BLOCK || node->ty == ND_FDEF) {
      env = node->env;
   }

   if (node->lhs) {
      node->lhs = analyzing(node->lhs);
   }
   if (node->rhs) {
      node->rhs = analyzing(node->rhs);
   }
   if (node->code) {
      for (j = 0; j < node->code->len && node->code->data[j]; j++) {
         node->code->data[j] = (Token *)analyzing((Node *)node->code->data[j]);
      }
   }
   if (node->argc > 0) {
      for (j = 0; j < node->argc; j++) {
         if (node->args[j]) {
            node->args[j] = analyzing(node->args[j]);
         }
      }
   }

   for (j = 0; j < 3; j++) {
      if (node->conds[j]) {
         node->conds[j] = analyzing(node->conds[j]);
      }
   }

   switch (node->ty) {
      case ND_ADD:
      case ND_SUB:
         if (node->lhs->type->ty == TY_PTR || node->lhs->type->ty == TY_ARRAY) {
            node->rhs = new_node(ND_MULTIPLY_IMMUTABLE_VALUE, node->rhs, NULL);
            node->rhs->num_val = cnt_size(node->lhs->type->ptrof);
         } else if (node->rhs->type->ty == TY_PTR ||
                    node->rhs->type->ty == TY_ARRAY) {
            node->lhs = new_node(ND_MULTIPLY_IMMUTABLE_VALUE, node->lhs, NULL);
            node->lhs->num_val = cnt_size(node->rhs->type->ptrof);
         } else {
            // Cast to node->type
            if (type2size(node->type) != type2size(node->lhs->type)) {
               node->lhs = new_node(ND_CAST, node->lhs, NULL);
               node->lhs->type = node->rhs->type;
            }
            if (type2size(node->type) != type2size(node->rhs->type)) {
               node->rhs = new_node(ND_CAST, node->rhs, NULL);
               node->rhs->type = node->lhs->type;
            }
         }
         if (node->rhs->type->ty == TY_PTR || node->rhs->type->ty == TY_ARRAY) {
            node->type = node->rhs->type;
            // TY_PTR no matter when node->lhs->node->lhs is INT or PTR
         } else if (node->lhs->type->ty == TY_PTR ||
                    node->lhs->type->ty == TY_ARRAY) {
            node->type = node->lhs->type;
            // TY_PTR no matter when node->lhs->node->lhs is INT or PTR
         }
         break;
      case '<':
      case '>':
      case ND_ISEQ:
      case ND_ISNOTEQ:
      case ND_ISLESSEQ:
      case ND_ISMOREEQ:
      case ND_MUL: {
         // TODO This has problem like: long + pointer -> pointer + pointer
         if (type2size(node->lhs->type) < type2size(node->rhs->type)) {
            node->lhs = new_node(ND_CAST, node->lhs, NULL);
            node->lhs->type = node->rhs->type;
         } else if (type2size(node->lhs->type) > type2size(node->rhs->type)) {
            node->rhs = new_node(ND_CAST, node->rhs, NULL);
            node->rhs->type = node->lhs->type;
         }
      } break;
      case ND_FPLUSPLUS:
      case ND_FSUBSUB:
         // Moved to analyzing process.
         if (node->lhs->type->ty == TY_PTR || node->lhs->type->ty == TY_ARRAY) {
            node->num_val = type2size(node->lhs->type->ptrof);
         }
         break;
      case ND_DEREF:
         // FIXME Just adding it on analyzing pharase will fail to selfhosting.
         node->type = node->lhs->type->ptrof;
         if (!node->type) {
            error("Error: Dereference on NOT pointered.");
         }
         break;
      case ND_ASSIGN:
         if (type2size(node->lhs->type) != type2size(node->rhs->type)) {
            node->rhs = new_node(ND_CAST, node->rhs, NULL);
            node->rhs->type = node->lhs->type;
         }
         break;
      case ND_DOT:
         // FIXME Just adding it on analyzing pharase will fail to selfhosting.
         node->type = (Type *)map_get(node->lhs->type->structure, node->name);
         if (!node->type) {
            error("Error: structure not found.");
         }
         if (node->lhs->type->ty != TY_STRUCT) {
            fprintf(stderr, "Error: dot operator to NOT struct\n");
            exit(1);
         }
         break;
      default:
         break;
   }

   env = prev_env;
   return node;
}

Node *optimizing(Node *node) {
   Node *node_new;
   int new_num_val;
   int j;
   switch (node->ty) {
      case ND_ADD:
      case ND_SUB:
      case ND_MUL:
         if (node->lhs->ty == ND_NUM && node->rhs->ty == ND_NUM) {
            switch (node->ty) {
               case ND_ADD:
                  new_num_val = node->lhs->num_val + node->rhs->num_val;
                  break;
               case ND_SUB:
                  new_num_val = node->lhs->num_val - node->rhs->num_val;
                  break;
               case ND_MUL:
                  new_num_val = node->lhs->num_val * node->rhs->num_val;
                  break;
               default:
                  fprintf(stderr, "Error: Uncalled\n");
                  exit(1);
            }
            node_new = new_num_node(new_num_val);
            node_new->type = node->lhs->type;
            node = node_new;
         }
         break;

      case ND_FUNC:
         if (!node->funcdef) {
            break;
         }

         // inline expansion
         /*
         if ((node->funcdef->code->len > 0) && (node->funcdef->code->len < 2) &&
         (node->funcdef->is_recursive == 0) && (node->type->ty == TY_VOID)) {

            node_new = new_block_node(NULL);
            for (j = 0;j < node->argc;j++) {
               node_new->args[j] = new_node(ND_ASSIGN, node->funcdef->args[j],
         node->args[j]);
            }
            if (node_new) {
               // constant function
               node_new->ty = ND_EXPRESSION_BLOCK;
               node_new->code = node->funcdef->code;
               node_new->env = node->funcdef->env;
               node_new->argc = node->argc;
               node = node_new;
            }
         }
         */
         break;
      default:
         break;
   }

   if (node->lhs) {
      node->lhs = optimizing(node->lhs);
   }
   if (node->rhs) {
      node->rhs = optimizing(node->rhs);
   }
   if (node->code) {
      for (j = 0; j < node->code->len && node->code->data[j]; j++) {
         node->code->data[j] = (Token *)optimizing((Node *)node->code->data[j]);
      }
   }
   if (node->argc > 0) {
      for (j = 0; j < node->argc; j++) {
         if (node->args[j]) {
            node->args[j] = optimizing(node->args[j]);
         }
      }
   }

   for (j = 0; j < 3; j++) {
      if (node->conds[j]) {
         node->conds[j] = optimizing(node->conds[j]);
      }
   }
   return node;
}

void analyzing_process() {
   int len = globalcode->len;
   int i;
   for (i = 0; i < len; i++) {
      globalcode->data[i] = (Token *)analyzing((Node *)globalcode->data[i]);
   }
}

void optimizing_process() {
   int len = globalcode->len;
   int i;
   for (i = 0; i < len; i++) {
      globalcode->data[i] = (Token *)optimizing((Node *)globalcode->data[i]);
   }
}

int main(int argc, char **argv) {
   if (argc < 2) {
      error("Incorrect Arguments");
   }
   test_map();

   tokens = new_vector();
   int is_from_file = 0;
   int is_register = 1;
   int i;
   for (i = argc - 2; i >= 1; i--) {
      if (strcmp(argv[i], "-f") == 0) {
         is_from_file = 1;
      } else if (strcmp(argv[i], "-r") == 0) {
         is_register = 1;
      } else if (strcmp(argv[i], "-cpp") == 0) {
         lang |= 1;
      } else if (strcmp(argv[i], "-objc") == 0) {
         lang |= 2;
      }
   }
   if (is_from_file) {
      preprocess(read_tokenize(argv[argc - 1]));
   } else {
      preprocess(tokenize(argv[argc - 1]));
   }

   init_typedb();

   toplevel();

   analyzing_process();

   optimizing_process();

   puts(".intel_syntax noprefix");
   puts(".align 4");
   if (is_from_file) {
      // 1 means the file number
      printf(".file 1 \"%s\"\n", argv[argc - 1]);
   } else {
      printf(".file 1 \"tmp.c\"\n");
   }
   // treat with global variables
   globalvar_gen();

   puts(".text");
   if (is_register) {
      gen_register_top();
   }

   return 0;
}
