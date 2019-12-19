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
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEEK_END 2
#define SEEK_SET 0

#define SPACE_R12R13R14R15 32

#define NO_REGISTER NULL

char *strdup(const char *s);

// to use vector instead of something
Vector *tokens;
int pos = 0;
Vector *globalcode;
Vector *strs;
Vector *floats;
Vector *float_doubles;
Map *global_vars;
Map *funcdefs_generated_template;
Map *funcdefs;
Map *consts;
Map *typedb;
Map *struct_typedb;
Map *current_local_typedb;
int is_recursive(Node *node, char *name);

int env_for_while = 0;
int env_for_while_switch = 0;
Env *env;
int if_cnt = 0;
int for_while_cnt = 0;
int neg_float_generate = 0;
int neg_double_generate = 0;

char arg_registers[6][4];

int lang = 0;
int output = 0;

Node *new_func_node(Node *ident, Vector *template_types, Vector *args);
Node *new_node(NodeType ty, Node *lhs, Node *rhs);
int type2size(Type *type);
Type *read_type_all(char **input);
Type *read_type(Type *type, char **input, Map *local_typedb);
Type *struct_declartion(char* struct_name);
Type *read_fundamental_type(Map *local_typedb);
int confirm_type();
int confirm_token(TokenConst ty);
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
Type *find_typed_db_without_copy(char *input, Map *db);
Type *class_declaration(Map *local_typedb);
int cnt_size(Type *type);
LocalVariable *get_type_local(Node *node);
LocalVariable *new_local_variable(char *name, Type *type);
Node *generate_template(Node *node, Map *template_type_db);
Type *generate_class_template(Type *type, Map *template_type_db);

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
Node *node_unary();
Node *assign();
_Noreturn void error(const char *str, ...);

Node *analyzing(Node *node);
Node *optimizing(Node *node);

// FOR SELFHOST
#ifdef __HANANDO_FUKUI__
FILE *fopen(char *name, char *type);
void *malloc(int size);
void *realloc(void *ptr, int size);
float strtof(char *nptr, char **endptr);
double strtod(char *nptr, char **endptr);
char *dirname(char *path);
char *basename(char *path);
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
      case TY_DOUBLE:
         return "double";
      case TY_PTR:
      case TY_ARRAY:
         sprintf(buf, "%s_ptr", type2name(type->ptrof));
         return buf;
      case TY_STRUCT:
         if (type->type_name) {
            sprintf(buf, "%s_struct", type->type_name);
         } else {
            sprintf(buf, "nonname_struct");
         }
         return buf;
      default:
         return "type";
   }
}

Type *new_type(void) {
   Type *type = malloc(sizeof(Type));
   type->ptrof = NULL;
   type->ret = NULL;
   type->argc = 0;
   type->array_size = 0;
   type->initval = 0;
   type->initstr = NULL;
   type->offset = 0;
   type->is_const = 0;
   type->is_static = 0;
   type->is_omiited = 0;
   type->type_name = NULL;
   type->context = malloc(sizeof(Type));
   type->context->is_previous_class = NULL;
   type->context->method_name = NULL;
   type->local_typedb = NULL;
   type->base_class = NULL;
   type->is_extern = 0;
   return type;
}

Type *type_pointer(Type *base_type) {
   Type *type = new_type();
   type->ty = TY_PTR;
   type->ptrof = base_type;
   return type;
}

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
   node->float_val = 0.0;
   node->name = NULL;
   node->gen_name = NULL;
   node->env = NULL;
   node->type = NULL;
   node->is_omiited = NULL;
   node->is_static = 0;
   node->is_recursive = 0;
   node->args[0] = NULL;
   node->args[1] = NULL;
   node->args[2] = NULL;
   node->args[3] = NULL;
   node->args[4] = NULL;
   node->args[5] = NULL;
   node->pline = -1; // more advenced text
   node->funcdef = NULL;
   node->sizeof_type = NULL;
   node->local_variable = NULL;

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
   return node;
}

Node *new_num_node_from_token(Token *token) {
   Node *node = new_node(ND_NUM, NULL, NULL);
   node->num_val = token->num_val;
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
   node->type = find_typed_db("int", typedb);
   return node;
}
Node *new_long_num_node(long num_val) {
   Node *node = new_node(ND_NUM, NULL, NULL);
   node->ty = ND_NUM;
   node->num_val = num_val;
   node->type = find_typed_db("long", typedb);
   return node;
}
Node *new_float_node(double num_val, char *_buf, char *typename_float) {
   // typename: should be "float" or "double"
   Node *node = new_node(ND_FLOAT, NULL, NULL);
   node->ty = ND_FLOAT;
   node->num_val = num_val;
   node->name = _buf;
   node->type = find_typed_db(typename_float, typedb);
   return node;
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
   node->local_variable = new_local_variable(name, type);
   return node;
}

Node *new_ident_node(char *name) {
   Node *node = new_node(ND_IDENT, NULL, NULL);
   node->name = name;
   if (strcmp(node->name, "stderr") == 0) {
      // TODO dirty
      node->ty = ND_EXTERN_SYMBOL;
      node->type = new_type();
      node->type->ty = TY_PTR;
      node->type->ptrof = new_type();
      node->type->ptrof->ty = TY_INT;
      return node;
   }
   LocalVariable *var = get_type_local(node);
   if (var) {
      node->type = var->type;
      node->local_variable = var;
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
   if (confirm_token('(')) {
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

Node *new_func_node(Node *ident, Vector *template_types, Vector *args) {
   Node *node = new_node(ND_FUNC, ident, NULL);
   Map *template_type_db = new_map();
   node->type = NULL;
   node->argc = 0;
   Type *template_type = NULL;
   int j;
   char *name = malloc(sizeof(char) * 256);
   if (ident->ty == ND_DOT) {
      // TODO Dirty: Add "this"
      node->args[0] = new_node(ND_ADDRESS, ident->lhs, NULL);
      node->argc = 1;
   }

   // insert args
   for (j = 0; j < args->len; j++) {
      node->args[j + node->argc] = (Node *)args->data[j];
   }
   node->argc += j;

   if (node->argc > 6) {
      error("Error: Too many arguments: %d\n", node->argc);
   }

   if (ident->ty == ND_SYMBOL || ident->ty == ND_DOT) {
      if (ident->ty == ND_SYMBOL) {
         name = ident->name;
      } else if (ident->ty == ND_DOT) {
         sprintf(name, "%s::%s", ident->lhs->type->type_name, ident->name);
      }
      node->name = name;
      node->gen_name = mangle_func_name(node->name);
      Node *result = map_get(funcdefs, node->name);
      if (result) {
         node->funcdef = result;
         // use the function definition.
         if (template_types && template_types->len > 0) {
            if (template_types->len != result->type->local_typedb->keys->len) {
               error("Error: The number of template arguments does not match: "
                     "Expected %d, Actual %d",
                     result->type->local_typedb->keys->len,
                     template_types->len);
            }
            // Make connection from template_types to local_typedb

            char *buf = malloc(sizeof(char) * 1024);
            sprintf(buf, "%s", ident->name);
            for (j = 0; j < result->type->local_typedb->keys->len; j++) {
               template_type =
                   (Type *)result->type->local_typedb->vals->data[j];
               map_put(template_type_db, template_type->type_name,
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
               node->funcdef = result;
            } else {
               node->funcdef = map_get(funcdefs_generated_template, buf);
            }
         }
      }
   } else {
      // something too complex
      // so add into function node & call
      node->name = NULL;
      node->gen_name = NULL;
      node->type = ident->type;
   }
   return node;
}

Env *new_env(Env *prev_env, Type *ret) {
   Env *_env = malloc(sizeof(Env));
   _env->env = prev_env;
   _env->idents = new_map();
   _env->rsp_offset = SPACE_R12R13R14R15; // to save r12, r13, r14, r15
   _env->ret = ret;
   _env->current_class = NULL;
   _env->current_func = NULL;
   if (prev_env) {
      _env->rsp_offset = prev_env->rsp_offset;
      _env->rsp_offset_max = prev_env->rsp_offset_max;
      _env->current_class =
          prev_env->current_class; // propagate current class info.
      _env->current_func =
          prev_env->current_func; // propagate current func info.
      if (!ret) {
         // env->ret will be propagated as ND_BLOCK.
         _env->ret = prev_env->ret;
      }
   } else {
      _env->rsp_offset_max = malloc(sizeof(int));
      *_env->rsp_offset_max = SPACE_R12R13R14R15; // to save r12, r13, r14, r15
   }
   return _env;
}

Node *new_fdef_node(char *name, Type *type, int is_static) {
   Node *node = new_node(ND_FDEF, NULL, NULL);
   node->type = type;
   node->name = name;
   node->argc = 0;
   node->env = new_env(NULL, type->ret);
   node->code = new_vector();
   node->is_omiited = NULL; // func(a, b, ...)
   node->is_static = is_static;
   node->gen_name = mangle_func_name(name);
   map_put(funcdefs, name, node);
   return node;
}

Node *new_block_node(Env *prev_env) {
   Node *node = new_node(ND_BLOCK, NULL, NULL);
   node->argc = 0;
   node->type = NULL;
   node->env = new_env(prev_env, NULL);
   node->code = new_vector();
   return node;
}

int confirm_token(TokenConst ty) {
   if (tokens->data[pos]->ty != ty) {
      return 0;
   }
   return 1;
}

int consume_token(TokenConst ty) {
   if (tokens->data[pos]->ty != ty) {
      return 0;
   }
   pos++;
   return 1;
}

TokenConst expect_token_or_token(TokenConst ty1, TokenConst ty2) {
   if (tokens->data[pos]->ty == ty1) {
      pos++;
      return ty1;
   } else if (tokens->data[pos]->ty == ty2) {
      pos++;
      return ty2;
   } else {
      error(
          "Error: Expected TokenConst are different: Expected %d/%d, Actual %d",
          ty1, ty2, tokens->data[pos]->ty);
      return 0;
   }
}

int expect_token(TokenConst ty) {
   if (tokens->data[pos]->ty != ty) {
      error("Error: Expected TokenConst are different: Expected %d, Actual %d",
            ty, tokens->data[pos]->ty);
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

Node *node_mathexpr_without_comma(void) { return node_lor(); }

Node *node_mathexpr(void) {
   Node *node = node_lor();
   while (1) {
      if (consume_token(',')) {
         node = new_node(',', node, node_lor());
      } else {
         return node;
      }
   }
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

Node *node_lor(void) {
   Node *node = node_land();
   while (1) {
      if (consume_token(TK_OR)) {
         node = new_node(ND_LOR, node, node_land());
      } else {
         return node;
      }
   }
}

Node *node_shift(void) {
   Node *node = node_add();
   // TODO: Shift should be only char.
   while (1) {
      if (consume_token(TK_LSHIFT)) {
         node = new_node(ND_LSHIFT, node, node_add());
      } else if (consume_token(TK_RSHIFT)) {
         node = new_node(ND_RSHIFT, node, node_add());
      } else {
         return node;
      }
   }
}
Node *node_or(void) {
   Node *node = node_xor();
   while (1) {
      if (consume_token('|')) {
         node = new_node('|', node, node_xor());
      } else {
         return node;
      }
   }
}
Node *node_compare(void) {
   Node *node = node_shift();
   while (1) {
      if (consume_token('<')) {
         node = new_node_with_cast('<', node, node_shift());
      } else if (consume_token('>')) {
         node = new_node_with_cast('>', node, node_shift());
      } else if (consume_token(TK_ISLESSEQ)) {
         node = new_node_with_cast(ND_ISLESSEQ, node, node_shift());
      } else if (consume_token(TK_ISMOREEQ)) {
         node = new_node_with_cast(ND_ISMOREEQ, node, node_shift());
      } else {
         return node;
      }
   }
}

Node *node_iseq(void) {
   Node *node = node_compare();
   while (1) {
      if (consume_token(TK_ISEQ)) {
         node = new_node_with_cast(ND_ISEQ, node, node_compare());
      } else if (consume_token(TK_ISNOTEQ)) {
         node = new_node_with_cast(ND_ISNOTEQ, node, node_compare());
      } else {
         return node;
      }
   }
}

Node *node_and(void) {
   Node *node = node_iseq();
   while (1) {
      if (consume_token('&')) {
         node = new_node('&', node, node_iseq());
      } else {
         return node;
      }
   }
}
Node *node_xor(void) {
   Node *node = node_and();
   while (1) {
      if (consume_token('^')) {
         node = new_node('^', node, node_and());
      } else {
         return node;
      }
   }
}
Node *node_add(void) {
   Node *node = node_mul();
   while (1) {
      if (consume_token('+')) {
         node = new_node('+', node, node_mul());
      } else if (consume_token('-')) {
         node = new_node('-', node, node_mul());
      } else {
         return node;
      }
   }
}

Node *node_cast(void) {
   // reading cast stmt. This does not support multiple cast.
   Node *node = NULL;
   if (consume_token('(')) {
      if (confirm_type()) {
         Type *type = read_type_all(NULL);
         expect_token(')');
         node = new_node(ND_CAST, node_unary(), NULL);
         node->type = type;
         return node;
      }
      // because of consume_token'('
      // dirty
      pos--;
   }

   return node_unary();
}

Node *node_unary(void) {
   Node *node;
   if (consume_token('-')) {
      node = new_node(ND_NEG, node_unary(), NULL);
      return node;
   } else if (consume_token('!')) {
      node = new_node('!', node_unary(), NULL);
      return node;
   } else if (consume_token('+')) {
      node = node_unary();
      return node;
   } else if (consume_token(TK_PLUSPLUS)) {
      node = new_ident_node(expect_ident());
      node = new_assign_node(node, new_node('+', node, new_num_node(1)));
   } else if (consume_token(TK_SUBSUB)) {
      node = new_ident_node(expect_ident());
      node = new_assign_node(node, new_node('-', node, new_num_node(1)));
   } else if (consume_token('&')) {
      node = node_unary();
      if (node->ty == ND_DEREF) {
         node = node->lhs;
      } else {
         node = new_node(ND_ADDRESS, node, NULL);
      }
   } else if (consume_token('*')) {
      node = new_node(ND_DEREF, node_unary(), NULL);
   } else if (consume_token(TK_SIZEOF)) {
      node = new_node(ND_SIZEOF, NULL, NULL);
      node->type = find_typed_db("long", typedb);
      if (consume_token('(')) {
         // should be type
         node->sizeof_type = read_type_all(NULL);
         expect_token(')');
      } else {
         // evaluate the result of ND_SIZEOF
         node->conds[0] = node_mathexpr();
      }
   } else if ((lang & 1) && consume_token(TK_NEW)) {
      // 5.3.4 New
      // unary-expression | new_expression:
      //    new  new-type-id new-initializer_opt
      // new-type-id:
      //    type-specifier
      // new-iniitalizer:
      //    ( expression-list)
      Type *newtype = read_fundamental_type(NULL);
      Map *local_typedb = new_map();
      newtype = read_type(newtype, NULL, local_typedb); // not expected type
      Vector *args = new_vector();
      if (consume_token('(')) {
         while (1) {
            if ((consume_token(',') == 0) && consume_token(')')) {
               break;
            }
            vec_push(args, (Token *)node_mathexpr_without_comma());
         }
      }
      node = new_func_node(node, newtype, args);
   } else {
      return node_term();
   }
   return node;
}

Node *node_mul(void) {
   Node *node = node_cast();
   while (1) {
      if (consume_token('*')) {
         node = new_node_with_cast('*', node, node_cast());
      } else if (consume_token('/')) {
         node = new_node('/', node, node_cast());
      } else if (consume_token('%')) {
         node = new_node('%', node, node_cast());
      } else {
         return node;
      }
   }
}

Node *new_dot_node(Node *node) {
   node = new_node('.', node, NULL);
   node->name = expect_ident();
   return node;
}

Node *treat_va_start(void) {
   Node *node = new_node(ND_VASTART, NULL, NULL);
   expect_token('(');
   node->lhs = node_term();
   node->rhs = new_ident_node("_saved_var");
   expect_token(',');
   node_term();
   expect_token(')');
   return node;
}

Node *treat_va_arg(void) {
   Node *node = new_node(ND_VAARG, NULL, NULL);
   expect_token('(');
   node->lhs = node_term();
   expect_token(',');
   // do not need ident name
   node->type = read_type_all(NULL);
   expect_token(')');
   return node;
}

Node *treat_va_end(void) {
   Node *node = new_node(ND_VAEND, NULL, NULL);
   expect_token('(');
   node->lhs = node_term();
   expect_token(')');
   return node;
}

Vector *read_template_argument_list(Map *local_typedb) {
   Vector *template_types = NULL;
   Type *template_type = NULL;
   expect_token('<');
   template_types = new_vector();
   while (1) {
      template_type = read_fundamental_type(local_typedb);
      template_type = read_type(template_type, NULL, local_typedb);
      // In this position, we don't know definition of template_type_db
      vec_push(template_types, (Token *)template_type);
      if (!consume_token(',')) {
         expect_token('>');
         break;
      }
   }
   return template_types;
}

Node *node_term(void) {
   Node *node = NULL;
   Vector *template_types = NULL;
   int j;

   // Primary Expression
   if (confirm_token(TK_FLOAT)) {
      char *_str = malloc(sizeof(char) * 256);
      float *float_repr = malloc(sizeof(float));
      *float_repr = (float)tokens->data[pos]->float_val;
      snprintf(_str, 255, ".LCF%d", vec_push(floats, (Token *)float_repr));
      node = new_float_node(tokens->data[pos]->float_val, _str, "float");
      expect_token(TK_FLOAT);
   } else if (confirm_token(TK_DOUBLE)) {
      char *_str = malloc(sizeof(char) * 256);
      snprintf(_str, 255, ".LCD%d",
               vec_push(float_doubles, (Token *)&tokens->data[pos]->float_val));
      node = new_float_node(tokens->data[pos]->float_val, _str, "double");
      expect_token(TK_DOUBLE);
   } else if (confirm_token(TK_NUM)) {
      node = new_num_node(tokens->data[pos]->num_val);
      expect_token(TK_NUM);
   } else if (consume_token(TK_NULL)) {
      node = new_num_node(0);
      node->type->ty = TY_PTR;
   } else if (confirm_ident()) {
      // treat constant or variable
      char *input = expect_ident();
      for (j = 0; j < consts->keys->len; j++) {
         // support for constant
         if (strcmp((char *)consts->keys->data[j], input) == 0) {
            node = (Node *)consts->vals->data[j];
            return node;
         }
      }
      node = new_ident_node(input);
   } else if (consume_token('(')) {
      node = assign();
      if (consume_token(')') == 0) {
         error("Error: Incorrect Parensis.");
      }
   } else if (confirm_token(TK_STRING)) {
      char *_str = malloc(sizeof(char) * 256);
      snprintf(_str, 255, ".LC%d",
               vec_push(strs, (Token *)tokens->data[pos]->input));
      node = new_string_node(_str);
      expect_token(TK_STRING);
   }

   if (!node) {
      error("Error: No Primary Expression.");
   }

   while (1) {
      // Postfix Expression

      // Template
      if ((lang & 1) && confirm_token('<')) {
         if (node->ty != ND_SYMBOL) {
            continue;
         }
         Node *result = map_get(funcdefs, node->name);
         if (!result || !result->type->local_typedb ||
             result->type->local_typedb->keys->len <= 0) {
            continue;
         }
         template_types = read_template_argument_list(NULL);
      } else if (confirm_token('(')) {
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

         // skip func , (
         expect_token('(');
         Vector *args = new_vector();
         while (1) {
            if ((consume_token(',') == 0) && consume_token(')')) {
               break;
            }
            vec_push(args, (Token *)node_mathexpr_without_comma());
         }
         node = new_func_node(node, template_types, args);
         // assert(node->argc <= 6);
         // pos++ because of consume_token(')')
         // return node;
      } else if (consume_token(TK_PLUSPLUS)) {
         node = new_node(ND_FPLUSPLUS, node, NULL);
         node->num_val = 1;
         // if (node->lhs->type->ty == TY_PTR || node->lhs->type->ty ==
         // TY_ARRAY) { Moved to analyzing process.
      } else if (consume_token(TK_SUBSUB)) {
         node = new_node(ND_FSUBSUB, node, NULL);
         node->num_val = 1;
         // if (node->lhs->type->ty == TY_PTR || node->lhs->type->ty ==
         // TY_ARRAY) { Moved to analyzing process.
      } else if (consume_token('[')) {
         node = new_node(ND_DEREF, new_node('+', node, node_mathexpr()), NULL);
         expect_token(']');
      } else if (consume_token('.')) {
         node = new_dot_node(node);
      } else if (consume_token(TK_ARROW)) {
         node = new_dot_node(new_node(ND_DEREF, node, NULL));
      } else {
         return node;
      }
   }
}

LocalVariable *get_type_local(Node *node) {
   Env *local_env = env;
   LocalVariable *local_variable = NULL;
   while (local_variable == NULL && local_env != NULL) {
      local_variable = map_get(local_env->idents, node->name);
      if (local_variable) {
         node->env = local_env;
         return local_variable;
      }
      local_env = local_env->env;
   }
   return local_variable;
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
      secure_mutable_with_type(lhs_reg, node->lhs->type);
   if (node->lhs->type->ty == TY_FLOAT) {
      return printf("comiss %s, %s\n", node2reg(node->lhs, lhs_reg),
                    node2reg(node->rhs, rhs_reg));
   } else if (node->lhs->type->ty == TY_DOUBLE) {
      return printf("comisd %s, %s\n", node2reg(node->lhs, lhs_reg),
                    node2reg(node->rhs, rhs_reg));
   } else if (type2size(node->lhs->type) < type2size(node->rhs->type)) {
      // rdi, rax
      return printf("cmp %s, %s\n", node2reg(node->rhs, lhs_reg),
                    node2reg(node->rhs, rhs_reg));
   } else {
      return printf("cmp %s, %s\n", node2reg(node->lhs, lhs_reg),
                    node2reg(node->lhs, rhs_reg));
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
      case TY_DOUBLE:
         return 8;
      case TY_CHAR:
         return 1;
      case TY_ARRAY:
         return 8;
      case TY_STRUCT:
         return cnt_size(type);
      case TY_FUNC:
         return 8;
      default:
         error("Error: NOT a type: %d", type->ty);
         return 0;
   }
}

int cnt_size(Type *type) {
   switch (type->ty) {
      case TY_PTR:
      case TY_INT:
      case TY_FLOAT:
      case TY_DOUBLE:
      case TY_CHAR:
      case TY_LONG:
      case TY_FUNC:
         return type2size(type);
      case TY_ARRAY:
         return cnt_size(type->ptrof) * type->array_size;
      case TY_STRUCT:
         // TODO its dirty
         // As possible, propagate (previous) type->offset as you can
         // TODO it causes bug when "Type" are duplicated typedb & struct_typedb
         // buf different
         // TODO it causes a bug when nested struct like:
         // struct {
         // struct {
         // }
         // } because offset are ALSO used as other ones
         if (type->offset > 0) {
            return type->offset;
         }
         char *name = type->type_name;
         type = find_typed_db(name, typedb);
         if (!type) {
            type = find_typed_db(name, struct_typedb);
         }
         return type->offset;
      case TY_TEMPLATE:
         return 8;
      default:
         error("Error: NOT a type: %d", type->ty);
         return 0;
   }
}

int reg_table[6];
int float_reg_table[8];
char *registers8[5];
char *registers16[5];
char *registers32[5];
char *registers64[5];
char *float_registers[8];
char *float_arg_registers[6];

// These registers will be used to map into registers
void init_reg_registers(void) {
   // This code is valid (and safe) because RHS is const ptr. lreg[7] -> on top
   // of "r10b"
   // "r15" will be used as rsp alignment register.
   registers8[0] = "r12b";
   registers8[1] = "r13b";
   registers8[2] = "r14b";
   registers8[3] = "r11b";
   registers8[4] = "r10b";
   registers16[0] = "r12w";
   registers16[1] = "r13w";
   registers16[2] = "r14w";
   registers16[3] = "r11w";
   registers16[4] = "r10w";
   registers32[0] = "r12d";
   registers32[1] = "r13d";
   registers32[2] = "r14d";
   registers32[3] = "r11d";
   registers32[4] = "r10d";
   registers64[0] = "r12";
   registers64[1] = "r13";
   registers64[2] = "r14";
   registers64[3] = "r11";
   registers64[4] = "r10";
   float_arg_registers[0] = "xmm0";
   float_arg_registers[1] = "xmm1";
   float_arg_registers[2] = "xmm2";
   float_arg_registers[3] = "xmm3";
   float_arg_registers[4] = "xmm4";
   float_arg_registers[5] = "xmm5";
   float_registers[0] = "xmm6";
   float_registers[1] = "xmm7";
   float_registers[2] = "xmm8";
   float_registers[3] = "xmm9";
   float_registers[4] = "xmm10";
   float_registers[5] = "xmm11";
   float_registers[6] = "xmm12";
   float_registers[7] = "xmm13";
}

char *id2reg8(int id) { return registers8[id]; }

char *id2reg32(int id) { return registers32[id]; }

char *id2reg64(int id) { return registers64[id]; }

void init_reg_table(void) {
   int j;
   for (j = 0; j < 6; j++) { // j = 6 means r15
      reg_table[j] = -1;     // NEVER_USED REGISTERS
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

char *type2mov(Type *type) {
   switch (type->ty) {
      case TY_FLOAT:
         return "movss";
      case TY_DOUBLE:
         return "movsd";
      /*case TY_CHAR:
         return "movzx";*/
      default:
         return "mov";
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
   } else if (reg->kind == R_LVAR || reg->kind == R_REVERSED_LVAR) {
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
   } else if (reg->kind == R_XMM) {
      return float_registers[reg->id];
   }
   error("Error: Cannot Have Register\n");
}
char *node2reg(Node *node, Register *reg) {
   if (reg->kind == R_LVAR || reg->kind == R_REVERSED_LVAR) {
      return size2reg(reg->size, reg);
   }
   return size2reg(type2size(node->type), reg);
}

Register *float_retain_reg(void) {
   int j;
   for (j = 0; j < 8; j++) {
      if (float_reg_table[j] > 0)
         continue;
      float_reg_table[j] = 1;

      Register *reg = malloc(sizeof(Register));
      reg->id = j;
      reg->name = NULL;
      reg->kind = R_XMM;
      return reg;
   }
   error("No more float registers are avaliable\n");
}

Register *retain_reg(void) {
   int j;
   for (j = 0; j < 5; j++) {
      if (reg_table[j] > 0)
         continue;
      if (reg_table[j] < 0 && j < 3) {
      }
      reg_table[j] = 1;

      Register *reg = malloc(sizeof(Register));
      reg->id = j;
      reg->name = NULL;
      reg->kind = R_REGISTER;
      reg->size = -1;
      return reg;
   }
   error("No more registers are avaliable\n");
}

void release_reg(Register *reg) {
   if (!reg) {
      return;
   }
   if (reg->kind == R_REGISTER) {
      reg_table[reg->id] = 0; // USED REGISTER
   } else if (reg->kind == R_XMM) {
      float_reg_table[reg->id] = 0;
   }
   free(reg);
}

void release_all_reg(void) {
   int j;
   for (j = 0; j < 6; j++) {
      // j = 5 means r15
      reg_table[j] = -1;
   }
   for (j = 0; j < 8; j++) {
      float_reg_table[j] = -1;
   }
}

void secure_mutable_with_type(Register *reg, Type *type) {
   Register *new_reg;
   // to enable to change
   if (reg->kind != R_REGISTER && reg->kind != R_XMM) {
      if (type->ty == TY_FLOAT || type->ty == TY_DOUBLE) {
         new_reg = float_retain_reg();
         printf("%s %s, %s\n", type2mov(type),
                size2reg(type2size(type), new_reg), size2reg(reg->size, reg));
      } else {
         new_reg = retain_reg();
         if (reg->size <= 0) {
            printf("mov %s, %s\n", size2reg(8, new_reg), size2reg(8, reg));
         } else if (reg->size == 1) {
            printf("movzx %s, %s\n", size2reg(8, new_reg),
                   size2reg(reg->size, reg));
         } else {
            printf("mov %s, %s\n", size2reg(reg->size, new_reg),
                   size2reg(reg->size, reg));
         }
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
         if (node->local_variable->lvar_offset >= 0) {
            printf("lea %s, [rbp-%d]\n", id2reg64(temp_reg->id),
                   node->local_variable->lvar_offset);
         } else {
            printf("lea %s, [rbp-%d]\n", id2reg64(temp_reg->id),
                   -node->local_variable->lvar_offset);
         }
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

void restore_callee_reg(void) {
   // stored in retain_reg() or
   // puts("mov r15, [rbp-24]");in ND_FUNC
   int j;
   // because r12,...are callee-savevd
   for (j = 2; j >= 0; j--) {
      // only registers already stored will be restoed
      if (reg_table[j] >= 0) { // used (>= 1)
         printf("mov %s, qword ptr [rbp-%d]\n", registers64[j], j * 8 + 8);
      }
   }
   if (reg_table[5] > 0) { // treat as r15
      printf("mov rsp, r15\n");
      puts("mov r15, qword ptr [rbp-32]");
   }
}

int save_reg(void) {
   int j;
   int stored_cnt = 0;
   // because only r10 and r11 are caller-saved
   for (j = 3; j < 5; j++) {
      if (reg_table[j] <= 0)
         continue;
      printf("push %s\n", registers64[j]);
      stored_cnt++;
   }
   return stored_cnt;
}

int restore_reg(void) {
   int j;
   int restored_cnt = 0;
   // 4 because only r10 and r11 are caller-saved
   for (j = 4; j >= 3; j--) {
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
   int func_call_float_cnt = 0;
   int func_call_should_sub_rsp = 0;

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
         printf("%s %s, %s[rip]\n", type2mov(node->type),
                node2reg(node, temp_reg), node->name);
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
            if (node->local_variable->lvar_offset < 0) {
               temp_reg->id = -node->local_variable->lvar_offset;
               temp_reg->kind = R_REVERSED_LVAR;
            } else {
               temp_reg->id = node->local_variable->lvar_offset;
               temp_reg->kind = R_LVAR;
            }
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
         lhs_reg = gen_register_leftval(node->lhs);
         switch (node->type->ty) {
            case TY_ARRAY:
               if (node->type->offset > 0) {
                  printf("add %s, %d\n", size2reg(8, lhs_reg),
                         node->type->offset);
               }
               break;
            case TY_FLOAT:
            case TY_DOUBLE:
               temp_reg = float_retain_reg();
               printf("%s %s, [%s+%d]\n", type2mov(node->type),
                      node2reg(node, temp_reg), id2reg64(lhs_reg->id),
                      node->type->offset);
               release_reg(lhs_reg);
               return temp_reg;
            default:
               printf("mov %s, [%s+%d]\n", node2reg(node, lhs_reg),
                      id2reg64(lhs_reg->id), node->type->offset);
               break;
         }
         return lhs_reg;
      case ND_ASSIGN:
         // This behaivour can be revised. like [rbp-8+2]
         if (node->lhs->ty == ND_IDENT || node->lhs->ty == ND_GLOBAL_IDENT) {
            lhs_reg = gen_register_rightval(node->lhs, 0);
            rhs_reg = gen_register_rightval(node->rhs, 0);
            secure_mutable_with_type(rhs_reg, node->lhs->type);
            if (node->lhs->type->ty == TY_FLOAT ||
                node->lhs->type->ty == TY_DOUBLE) {
               printf("%s %s, %s\n", type2mov(node->rhs->type),
                      node2reg(node->lhs, lhs_reg),
                      node2reg(node->lhs, rhs_reg));
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
         } else if (node->lhs->ty == ND_DOT && node->lhs->type->offset > 0) {
            /*
            add r12, 152
            mov qword ptr [r12], r13
            */
            // can be written as
            /*
            mov qword ptr [r12+152], r13
            */
            lhs_reg = gen_register_leftval(node->lhs->lhs);
            if (node->rhs->ty == ND_NUM) {
               rhs_reg = NULL;
               // can be written as immutable value
               printf("%s %s [%s+%d], %ld\n", type2mov(node->type),
                      node2specifier(node), id2reg64(lhs_reg->id),
                      node->lhs->type->offset, node->rhs->num_val);
            } else {
               rhs_reg = gen_register_rightval(node->rhs, 0);
               secure_mutable_with_type(rhs_reg, node->rhs->type);
               printf("%s %s [%s+%d], %s\n", type2mov(node->type),
                      node2specifier(node), id2reg64(lhs_reg->id),
                      node->lhs->type->offset, node2reg(node->lhs, rhs_reg));
            }
            release_reg(lhs_reg);
            if (unused_eval) {
               release_reg(rhs_reg);
            }
            return rhs_reg;
         } else {
            lhs_reg = gen_register_leftval(node->lhs);
            rhs_reg = gen_register_rightval(node->rhs, 0);
            secure_mutable_with_type(rhs_reg, node->rhs->type);
            printf("%s %s [%s], %s\n", type2mov(node->type),
                   node2specifier(node), id2reg64(lhs_reg->id),
                   node2reg(node->lhs, rhs_reg));
            release_reg(lhs_reg);
            if (unused_eval) {
               release_reg(rhs_reg);
            }
            return rhs_reg;
         }

      case ND_CAST:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         if (node->type->ty == TY_FLOAT) {
            switch (node->lhs->type->ty) {
               case TY_INT:
                  temp_reg = float_retain_reg();
                  printf("cvtsi2ss %s, %s\n", node2reg(node, temp_reg),
                         node2reg(node->lhs, lhs_reg));
                  release_reg(lhs_reg);
                  return temp_reg;
               case TY_DOUBLE:
                  temp_reg = float_retain_reg();
                  printf("cvtsd2ss %s, %s\n", node2reg(node, temp_reg),
                         node2reg(node->lhs, lhs_reg));
                  release_reg(lhs_reg);
                  return temp_reg;
               default:
                  break;
            }
            return lhs_reg;
         }
         if (node->type->ty == TY_DOUBLE) {
            switch (node->lhs->type->ty) {
               case TY_INT:
                  temp_reg = float_retain_reg();
                  printf("cvtsi2sd %s, %s\n", node2reg(node, temp_reg),
                         node2reg(node->lhs, lhs_reg));
                  release_reg(lhs_reg);
                  return temp_reg;
               case TY_FLOAT:
                  temp_reg = float_retain_reg();
                  printf("cvtss2sd %s, %s\n", node2reg(node, temp_reg),
                         node2reg(node->lhs, lhs_reg));
                  release_reg(lhs_reg);
                  return temp_reg;
               default:
                  break;
            }
            return lhs_reg;
         }
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         if (node->type->ty == node->lhs->type->ty) {
            return lhs_reg;
         }
         switch (node->lhs->type->ty) {
            case TY_CHAR:
               // TODO treat as unsigned char.
               // for signed char, use movsx instead of.
               printf("movzx %s, %s\n", node2reg(node, lhs_reg),
                      id2reg8(lhs_reg->id));
               break;
            case TY_FLOAT:
               temp_reg = retain_reg();
               if (node->type->ty == TY_INT) {
                  // TODO It it not supported yet: to convert float -> char is
                  // not supported yet
                  printf("cvttss2si %s, %s\n", node2reg(node->lhs, temp_reg),
                         node2reg(node->lhs, lhs_reg));
               } else if (node->type->ty == TY_LONG) {
                  printf("cvttss2siq %s, %s\n", node2reg(node->lhs, temp_reg),
                         node2reg(node->lhs, lhs_reg));
               } else {
                  error("Error: float -> unknown type convert: %d\n",
                        node->type->ty);
               }
               release_reg(lhs_reg);
               return temp_reg;
            case TY_DOUBLE:
               temp_reg = retain_reg();
               if (node->type->ty == TY_INT) {
                  // TODO It it not supported yet: to convert float -> char is
                  // not supported yet
                  printf("cvttsd2si %s, %s\n", node2reg(node->lhs, temp_reg),
                         node2reg(node->lhs, lhs_reg));
               } else if (node->type->ty == TY_LONG) {
                  printf("cvttsd2siq %s, %s\n", node2reg(node->lhs, temp_reg),
                         node2reg(node->lhs, lhs_reg));
               } else {
                  error("Error: double -> unknown type convert: %d\n",
                        node->type->ty);
               }
               release_reg(lhs_reg);
               return temp_reg;
            default:
               break;
         }
         return lhs_reg;

      case ND_COMMA:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         release_reg(lhs_reg);
         return rhs_reg;

      case ND_ADD:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable_with_type(lhs_reg, node->type);
         if (node->lhs->type->ty == TY_FLOAT) {
            printf("addss %s, %s\n", node2reg(node, lhs_reg),
                   node2reg(node, rhs_reg));
         } else if (node->lhs->type->ty == TY_DOUBLE) {
            printf("addsd %s, %s\n", node2reg(node, lhs_reg),
                   node2reg(node, rhs_reg));
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
         secure_mutable_with_type(lhs_reg, node->type);
         if (node->lhs->type->ty == TY_FLOAT) {
            printf("subss %s, %s\n", node2reg(node, lhs_reg),
                   node2reg(node, rhs_reg));
         } else if (node->lhs->type->ty == TY_DOUBLE) {
            printf("subsd %s, %s\n", node2reg(node, lhs_reg),
                   node2reg(node, rhs_reg));
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
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         printf("imul %s, %ld\n", node2reg(node, lhs_reg), node->num_val);
         return lhs_reg;

      case ND_MUL:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable_with_type(lhs_reg, node->type);
         if (node->lhs->type->ty == TY_FLOAT) {
            printf("mulss %s, %s\n", node2reg(node, lhs_reg),
                   node2reg(node, rhs_reg));
         } else if (node->lhs->type->ty == TY_DOUBLE) {
            printf("mulsd %s, %s\n", node2reg(node, lhs_reg),
                   node2reg(node, rhs_reg));
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
         if (node->lhs->type->ty == TY_FLOAT) {
            secure_mutable_with_type(lhs_reg, node->lhs->type);
            printf("divss %s, %s\n", node2reg(node->lhs, lhs_reg),
                   node2reg(node->rhs, rhs_reg));
         } else if (node->lhs->type->ty == TY_DOUBLE) {
            secure_mutable_with_type(lhs_reg, node->lhs->type);
            printf("divsd %s, %s\n", node2reg(node->lhs, lhs_reg),
                   node2reg(node->rhs, rhs_reg));
         } else {
            // TODO should support char
            printf("mov %s, %s\n", _rax(node->lhs),
                   node2reg(node->lhs, lhs_reg));
            puts("cqo");
            printf("idiv %s\n", node2reg(node->rhs, rhs_reg));
            secure_mutable_with_type(lhs_reg, node->lhs->type);
            printf("mov %s, %s\n", node2reg(node->lhs, lhs_reg),
                   _rax(node->lhs));
         }
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
         secure_mutable_with_type(lhs_reg, node->lhs->type);
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
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         printf("xor %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         release_reg(rhs_reg);
         return lhs_reg;

      case ND_AND:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         printf("and %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         release_reg(rhs_reg);
         return lhs_reg;
         break;

      case ND_OR:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable_with_type(lhs_reg, node->lhs->type);
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
         puts("xor rcx, rcx");
         printf("mov cl, %s\n", size2reg(1, rhs_reg));
         release_reg(rhs_reg);
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         printf("sar %s, cl\n", node2reg(node->lhs, lhs_reg));
         return lhs_reg;

      case ND_LSHIFT:
         // SHOULD be int
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         // FIXME: for signed int (Arthmetric)
         // mov rdi[8] -> rax
         puts("xor rcx, rcx");
         printf("mov cl, %s\n", size2reg(1, rhs_reg));
         release_reg(rhs_reg);
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         printf("sal %s, cl\n", node2reg(node->lhs, lhs_reg));
         return lhs_reg;

      case ND_ISEQ:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         cmp_regs(node, lhs_reg, rhs_reg);
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
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         cmp_regs(node, lhs_reg, rhs_reg);
         if (node->lhs->type->ty == TY_FLOAT ||
             node->lhs->type->ty == TY_DOUBLE) {
            puts("seta al");
         } else {
            puts("setg al");
         }
         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_LESS:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);

         if (node->lhs->type->ty == TY_FLOAT ||
             node->lhs->type->ty == TY_DOUBLE) {
            secure_mutable_with_type(rhs_reg, node->rhs->type);
            // lhs_reg should be xmm, but rhs should be xmm or memory
            // lhs_reg and rhs_reg are reversed because of comisd
            cmp_regs(node, rhs_reg, lhs_reg);
            puts("seta al");
         } else {
            secure_mutable_with_type(rhs_reg, node->rhs->type);
            cmp_regs(node, lhs_reg, rhs_reg);
            puts("setl al");
         }

         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_ISMOREEQ:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         cmp_regs(node, lhs_reg, rhs_reg);
         if (node->lhs->type->ty == TY_FLOAT ||
             node->lhs->type->ty == TY_DOUBLE) {
            puts("setnb al");
         } else {
            puts("setge al");
         }
         puts("and al, 1");
         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_ISLESSEQ:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         if (node->lhs->type->ty == TY_FLOAT ||
             node->lhs->type->ty == TY_DOUBLE) {
            // lhs_reg should be xmm, but rhs should be xmm or memory
            // lhs_reg and rhs_reg are reversed because of comisd
            secure_mutable_with_type(rhs_reg, node->rhs->type);
            cmp_regs(node, rhs_reg, lhs_reg);
            puts("setnb al");
         } else {
            secure_mutable_with_type(lhs_reg, node->lhs->type);
            cmp_regs(node, lhs_reg, rhs_reg);
            puts("setle al");
         }
         puts("and al, 1");
         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_RETURN:
         if (node->lhs) {
            lhs_reg = gen_register_rightval(node->lhs, 0);
            if (node->lhs->type->ty == TY_FLOAT ||
                node->lhs->type->ty == TY_DOUBLE) {
               printf("%s xmm0, %s\n", type2mov(node->lhs->type),
                      node2reg(node->lhs, lhs_reg));
            } else {
               printf("mov %s, %s\n", _rax(node->lhs),
                      node2reg(node->lhs, lhs_reg));
            }
            release_reg(lhs_reg);
         }
         restore_callee_reg();
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
         secure_mutable_with_type(lhs_reg, node->type);
         switch (node->type->ty) {
            case TY_FLOAT:
               temp_reg = float_retain_reg();
               printf("movq %s, qword ptr .LCDNEGFLOAT[rip]\n",
                      node2reg(node, temp_reg));
               printf("xorps %s, %s\n", node2reg(node, lhs_reg),
                      node2reg(node, temp_reg));
               release_reg(temp_reg);
               break;
            case TY_DOUBLE:
               temp_reg = float_retain_reg();
               printf("movq %s, qword ptr .LCDNEGDOUBLE[rip]\n",
                      node2reg(node, temp_reg));
               printf("xorpd %s, %s\n", node2reg(node, lhs_reg),
                      node2reg(node, temp_reg));
               release_reg(temp_reg);
               break;
            default:
               printf("neg %s\n", node2reg(node, lhs_reg));
               break;
         }
         return lhs_reg;

      case ND_FPLUSPLUS:
         lhs_reg = gen_register_leftval(node->lhs);
         if (unused_eval) {
            secure_mutable_with_type(lhs_reg, node->lhs->type);
            printf("add %s [%s], %ld\n", node2specifier(node),
                   id2reg64(lhs_reg->id), node->num_val);
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            temp_reg = retain_reg();
            secure_mutable_with_type(lhs_reg, node->lhs->type);

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
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         if (unused_eval) {
            printf("sub %s [%s], %ld\n", node2specifier(node),
                   id2reg64(lhs_reg->id), node->num_val);
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            temp_reg = retain_reg();

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
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         if (type2size(node->type) == 1) {
            // when reading char, we should read just 1 byte
            printf("movzx %s, byte ptr [%s]\n", size2reg(4, lhs_reg),
                   size2reg(8, lhs_reg));
         } else if (node->type->ty == TY_FLOAT || node->type->ty == TY_DOUBLE) {
            temp_reg = float_retain_reg();
            printf("%s %s, [%s]\n", type2mov(node->type),
                   node2reg(node, temp_reg), size2reg(8, lhs_reg));
            release_reg(lhs_reg);
            return temp_reg;
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
         int is_entrypoint = !(strncmp(node->gen_name, "main", 5));
         int fdef_int_arguments = 0;
         int fdef_float_arguments = 0;
         j = 0;
         for (fdef_int_arguments = 0;
              (fdef_int_arguments + fdef_float_arguments) < node->argc; j++) {
            if (node->args[j]->type->ty == TY_FLOAT ||
                node->args[j]->type->ty == TY_DOUBLE) {
               temp_reg = gen_register_rightval(node->args[j], 0);
               printf("%s %s, %s\n", type2mov(node->args[j]->type),
                      node2reg(node->args[j], temp_reg),
                      float_arg_registers[fdef_float_arguments]);
               fdef_float_arguments++;
            } else {
               temp_reg = gen_register_rightval(node->args[j], 0);
               printf("mov %s, %s\n", size2reg(8, temp_reg),
                      arg_registers[fdef_int_arguments]);
               fdef_int_arguments++;
            }
         }
         if (node->is_omiited) {
            for (j = 0; j < 6; j++) {
               printf("mov [rbp-%d], %s\n",
                      node->is_omiited->local_variable->lvar_offset - j * 8,
                      arg_registers[j]);
            }
            for (j = 0; j < 8; j++) {
               printf("movaps [rbp-%d], xmm%d\n",
                      node->is_omiited->local_variable->lvar_offset - 8 * 6 -
                          j * 16,
                      j);
            }
         }
         printf("jmp .LFD%s\n", node->gen_name);
         printf(".LFB%s:\n", node->gen_name);
         for (j = 0; j < node->code->len; j++) {
            // read inside functions.
            gen_register_rightval((Node *)node->code->data[j], 1);
         }
         if (node->type->ret->ty == TY_VOID || is_entrypoint) {
            if (!is_entrypoint) {
               restore_callee_reg();
            } else {
               puts("mov rax, 0"); // main() function will return 0
            }
            puts("mov rsp, rbp");
            puts("pop rbp");
            puts("ret");
         }

         printf(".LFD%s:\n", node->gen_name);
         // r10, r11 are caller-saved so no need to consider
         // store callee saved registers. will be restored in
         // restore_callee_reg() r12 will be stored in [rbp-0] r13 will be
         // stored in [rbp-8] r14 will be stored in [rbp-16]
         for (j = 0; j < 3; j++) {
            if (reg_table[j] >= 0) { // used (>= 1)
               printf("mov qword ptr [rbp-%d], %s\n", j * 8 + 8,
                      registers64[j]);
            }
         }
         if (reg_table[5] > 0) {                 // treat as r15
            puts("mov qword ptr [rbp-32], r15"); // store r15 to [rbp-32]
         }
         printf("jmp .LFB%s\n", node->gen_name);
         // Release All Registers.
         release_all_reg();
         return NO_REGISTER;

      case ND_BLOCK:
         for (j = 0; j < node->code->len && node->code->data[j]; j++) {
            gen_register_rightval((Node *)node->code->data[j], 1);
         }
         return NO_REGISTER;

      case ND_EXPRESSION_BLOCK:
         if (node->argc > 0) {
            puts("mov rbp, rsp");
            printf("sub rsp, %d\n", *node->env->rsp_offset_max);
            puts("#test begin expansion");
            for (j = 0; j < node->argc; j++) {
               // temp_reg = gen_register_rightval(node->args[j], 0);
               gen_register_rightval(node->args[j], 1);
               puts("#test end expansion of args");
            }
         }
         for (j = 0; j < node->code->len && node->code->data[j]; j++) {
            gen_register_rightval((Node *)node->code->data[j], 1);
         }
         if (node->argc > 0) {
            puts("mov rsp, rbp");
            puts("#test end expansion ");
         }
         return NO_REGISTER;

      case ND_FUNC:
         for (j = 0; j < node->argc; j++) {
            temp_reg = gen_register_rightval(node->args[j], 0);
            if (node->args[j]->type->ty == TY_FLOAT ||
                node->args[j]->type->ty == TY_DOUBLE) {
               printf("%s %s, %s\n", type2mov(node->args[j]->type),
                      float_arg_registers[func_call_float_cnt],
                      node2reg(node->args[j], temp_reg));
               func_call_float_cnt++;
            } else if (node->args[j]->type->ty == TY_STRUCT) {
               int k = 0;
               switch (temp_reg->kind) {
                  case R_GVAR:
                     func_call_should_sub_rsp += type2size(node->args[j]->type);
                     for (k = type2size(node->args[j]->type) - 8; k >= 0;
                          k -= 8) {
                        printf("push qword ptr %s[rip+%d]\n", temp_reg->name,
                               k);
                     }
                     break;
                  case R_LVAR:
                     func_call_should_sub_rsp += type2size(node->args[j]->type);
                     for (k = type2size(node->args[j]->type) - 8; k >= 0;
                          k -= 8) {
                        printf("push qword ptr [rbp-%d]\n", temp_reg->id - k);
                     }
                     break;
                  default:
                     error("Error: Not IMplemented On ND_FUNC w/ TY_STRUCT\n");
               }
               // TODO This will create "mov eax, 1" even though no float
               // arugments
               func_call_float_cnt++;
            } else {
               secure_mutable_with_type(temp_reg, node->type);
               printf("push %s\n", id2reg64(temp_reg->id));
            }
            release_reg(temp_reg);
         }
         for (j = node->argc - 1 - func_call_float_cnt; j >= 0; j--) {
            // because of function call will break these registers
            printf("pop %s\n", arg_registers[j]);
         }

         save_reg();
         printf("mov eax, %d\n", func_call_float_cnt);

         if (reg_table[5] < 0) {
            reg_table[5] = 1;
         }
         puts("mov r15, rsp");
         puts("and rsp, -16");
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
            secure_mutable_with_type(temp_reg, node->lhs->type);
            printf("call %s\n", size2reg(8, temp_reg));
            release_reg(temp_reg);
         }
         puts("mov rsp, r15");
         if (func_call_should_sub_rsp > 0) {
            printf("sub rsp, -%d\n", func_call_should_sub_rsp);
         }
         restore_reg();

         if (node->type->ty == TY_VOID || unused_eval == 1) {
            return NO_REGISTER;
         } else if (node->type->ty == TY_FLOAT || node->type->ty == TY_DOUBLE) {
            temp_reg = float_retain_reg();
            // movaps just move all of xmmx
            printf("movaps %s, xmm0\n", node2reg(node, temp_reg));
            return temp_reg;
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
                // omitted_argc * 8
                node->num_val * 8);
         printf("mov dword ptr [%s+4], 304\n", id2reg64(lhs_reg->id));
         printf("lea rax, [rbp-%d]\n", node->rhs->local_variable->lvar_offset);
         printf("mov qword ptr [%s+16], rax\n", id2reg64(lhs_reg->id));
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
         error("Error: Incorrect Registers %d.\n", node->ty);
   }
   return NO_REGISTER;
}

void gen_register_top(void) {
   init_reg_table();
   init_reg_registers();
   int j;
   for (j = 0; j < globalcode->len; j++) {
      gen_register_rightval((Node *)globalcode->data[j], 1);
   }
}

Node *assign(void) {
   Node *node = node_mathexpr();
   if (consume_token('=')) {
      if (confirm_token(TK_OPAS)) {
         NodeType tp = tokens->data[pos]->input[0];
         expect_token(TK_OPAS);
         if (tp == '<') {
            tp = ND_LSHIFT;
         }
         if (tp == '>') {
            tp = ND_RSHIFT;
         }
         node = new_assign_node(node, new_node_with_cast(tp, node, assign()));
      } else {
         node = new_assign_node(node, assign());
      }
   }
   return node;
}

char *read_function_name(Map *local_typedb) {
   char *buf = malloc(sizeof(char) * 1024);
   Vector *template_types = NULL;
   buf[0] = '\n';
   strncpy(buf, expect_ident(), 256);
   while (1) {
      if (consume_token(TK_COLONCOLON)) {
         strncat(buf, "::", 3);
         strncat(buf, expect_ident(), 256);
      } else if (confirm_token('<')) {
         template_types = read_template_argument_list(local_typedb);
         {
            int j;
            for (j = 0; j < template_types->len; j++) {
               strncat(buf, "_Template_", 12);
               strncat(buf, type2name((Type *)template_types->data[j]), 128);
            }
         }
         // treat as class template
      } else {
         return buf;
      }
   }
   return buf;
}

Type *read_type(Type *type, char **input, Map *local_typedb) {
   int ptr_cnt = 0;
   int i = 0;
   Type *concrete_type;

   // Variable Definition.
   // consume is pointer or not
   while (consume_token('*')) {
      ptr_cnt++;
   }
   // There are input: there are ident names
   if (input) {
      // When there are its name or not
      *input = NULL;
      if (confirm_token(TK_IDENT)) {
         *input = tokens->data[pos]->input;
      } else if (consume_token('(')) {
         // functional pointer. declarator
         concrete_type = read_type(NULL, input, local_typedb);
         Type *base_type = concrete_type;
         while (base_type->ptrof != NULL) {
            base_type = base_type->ptrof;
         }
         type = concrete_type;
         expect_token(')');
      }
   } else {
      input = &tokens->data[pos]->input;
   }
   consume_token(TK_IDENT);

   for (i = 0; i < ptr_cnt; i++) {
      type = type_pointer(type);
   }
   // array
   while (1) {
      if (consume_token('[')) {
         Type *base_type = type;
         type = new_type();
         type->ty = TY_ARRAY;
         type->array_size = (int)tokens->data[pos]->num_val;
         type->ptrof = base_type;
         expect_token(TK_NUM);
         expect_token(']');
         Type *cur_ptr = type;
         // support for multi-dimensional array
         while (consume_token('[')) {
            // TODO: support NOT functioned type
            // ex. int a[4+7];
            cur_ptr->ptrof = new_type();
            cur_ptr->ptrof->ty = TY_ARRAY;
            cur_ptr->ptrof->array_size = (int)tokens->data[pos]->num_val;
            cur_ptr->ptrof->ptrof = base_type;
            expect_token(TK_NUM);
            expect_token(']');
            cur_ptr = cur_ptr->ptrof;
         }
      } else if (consume_token('(')) {
         // Function call.
         Type *concrete_type = new_type();
         concrete_type->ty = TY_FUNC;
         concrete_type->ret = type;
         concrete_type->argc = 0;
         concrete_type->is_omiited = 0;

         // Propagate is_static func or not
         concrete_type->is_static = type->is_static;

         // follow concrete_type if real type !is not supported yet!
         // to set...
         // BEFORE: TY_PTR -> TY_PTR -> TY_PTR -> TY_INT
         // AFTER: TY_FUNC
         type = concrete_type;

         // Special Case: int func(void)
         if (tokens->data[pos]->ty == TK_IDENT &&
             (strncmp(tokens->data[pos]->input, "void", 5) == 0) &&
             tokens->data[pos + 1]->ty == ')') {
            concrete_type->argc = 0;
            pos += 2;
            continue;
         }

         // treat as function.
         for (concrete_type->argc = 0;
              concrete_type->argc <= 6 && !consume_token(')');) {
            if (consume_token(TK_OMIITED)) {
               type->is_omiited = 1;
               expect_token(')');
               break;
            }
            char *buf;
            concrete_type->args[concrete_type->argc] =
                read_fundamental_type(local_typedb);
            concrete_type->args[concrete_type->argc] = read_type(
                concrete_type->args[concrete_type->argc], &buf, local_typedb);
            // Save its variable name, if any.
            concrete_type->args[concrete_type->argc++]->var_name = buf;
            consume_token(',');
         }
      } else {
         break;
      }
   }
   if (!type) {
      error("Expected type, but no type found\n");
   }
   return type;
}

Type *read_type_all(char **input) {
   Type *type = read_fundamental_type(NULL);
   type = read_type(type, input, NULL);
   return type;
}

Node *stmt(void) {
   Node *node = NULL;
   int pline = tokens->data[pos]->pline;
   int j;
   if (consume_token(TK_RETURN)) {
      if (confirm_token(';')) {
         // to support return; with void type
         node = new_node(ND_RETURN, NULL, NULL);
      } else {
         node = new_node(ND_RETURN, assign(), NULL);
      }
      // FIXME GOTO is not statement, expr.
   } else if (consume_token(TK_GOTO)) {
      node = new_node(ND_GOTO, NULL, NULL);
      node->name = expect_ident();
   } else if (consume_token(TK_BREAK)) {
      node = new_node(ND_BREAK, NULL, NULL);
   } else if (consume_token(TK_CONTINUE)) {
      node = new_node(ND_CONTINUE, NULL, NULL);
   } else if (consume_token(TK_THROW)) {
      node = new_node(ND_THROW, NULL, NULL);
   } else if (confirm_type()) {
      char *input = NULL;
      Type *fundamental_type = read_fundamental_type(current_local_typedb);
      Type *type;
      do {
         if (confirm_token(';')) {
            // example: int;
            break;
         }
         type = read_type(duplicate_type(fundamental_type), &input, NULL);
         node = new_ident_node_with_new_variable(input, type);
         // if there is int a =1;
         // C++ initializer = | ( expression-list )
         if ((lang & 1) && type->ty == TY_STRUCT) {
            Node *func_node = NULL;
            // Constructor with arguments
            func_node = new_node(ND_DOT, node, NULL);
            func_node->name = type->type_name;

            // Find Constructor
            // TODO Constructor of base_classes will not be called
            if (map_get(type->structure, type->type_name)) {
               Vector *args = new_vector();
               // add arguments
               if (consume_token('(')) {
                  while (1) {
                     if ((consume_token(',') == 0) && consume_token(')')) {
                        break;
                     }
                     vec_push(args, (Token *)node_mathexpr_without_comma());
                  }
               }
               node = new_func_node(func_node, NULL, args);
            }
         }
         if (consume_token('=')) {
            // on C++ brace-or-equal-initializer
            if (consume_token('{')) {
               Node *block_node;
               // initializer
               block_node = new_node(ND_EXPRESSION_BLOCK, NULL, NULL);
               block_node->type = NULL;
               block_node->code = new_vector();
               vec_push(block_node->code, (Token *)node);
               // initializer (6.7.9)
               // assignment-expression or {initializer_list or
               // designation-list} compiled as x[3] = {1,2,3} as x[0] = 100,
               // x[1] = 200, x[2] = 300,...
               for (j = 0; 1; j++) {
                  vec_push(
                      block_node->code,
                      (Token *)new_node(
                          ND_ASSIGN,
                          new_node(ND_DEREF,
                                   new_node(ND_ADD, node, new_long_num_node(j)),
                                   NULL),
                          node_lor()));
                  if (consume_token('}')) {
                     consume_token(',');
                     break;
                  } else {
                     expect_token(',');
                  }
               }
               node = block_node;

            } else {
               node = new_node('=', node, node_mathexpr());
            }
         }
         input = NULL;
      } while (consume_token(','));
   } else {
      node = assign();
   }
   expect_token(';');
   if (node) {
      // ex: empty declartion
      node->pline = pline;
   }
   return node;
}

Node *node_if(void) {
   Node *node = new_node(ND_IF, NULL, NULL);
   node->argc = 1;
   node->pline = tokens->data[pos]->pline;
   node->args[0] = assign();
   node->args[1] = NULL;
   // Suppress COndition

   if (confirm_token(TK_BLOCKBEGIN)) {
      node->lhs = new_block_node(env);
      program(node->lhs);
   } else {
      // without block
      node->lhs = stmt();
   }
   if (consume_token(TK_ELSE)) {
      if (confirm_token(TK_BLOCKBEGIN)) {
         node->rhs = new_block_node(env);
         program(node->rhs);
      } else if (consume_token(TK_IF)) {
         node->rhs = node_if();
      } else {
         node->rhs = stmt();
      }
   }
   return node;
}

void program(Node *block_node) {
   expect_token('{');
   Vector *args = block_node->code;
   Env *prev_env = env;
   env = block_node->env;
   while (!consume_token('}')) {
      if (confirm_token('{')) {
         Node *new_block = new_block_node(env);
         program(new_block);
         vec_push(args, (Token *)new_block);
         continue;
      }

      if (consume_token(TK_IF)) {
         vec_push(args, (Token *)node_if());
         continue;
      }
      if (consume_token(TK_WHILE)) {
         Node *while_node = new_node(ND_WHILE, node_mathexpr(), NULL);
         if (confirm_token(TK_BLOCKBEGIN)) {
            while_node->rhs = new_block_node(env);
            program(while_node->rhs);
         } else {
            while_node->rhs = stmt();
         }
         vec_push(args, (Token *)while_node);
         continue;
      }
      if (consume_token(TK_DO)) {
         Node *do_node = new_node(ND_DOWHILE, NULL, NULL);
         do_node->rhs = new_block_node(env);
         program(do_node->rhs);
         expect_token(TK_WHILE);
         do_node->lhs = node_mathexpr();
         expect_token(';');
         vec_push(args, (Token *)do_node);
         continue;
      }
      if (consume_token(TK_FOR)) {
         Node *for_node = new_node(ND_FOR, NULL, NULL);
         expect_token('(');
         // TODO: should be splited between definition and expression
         for_node->rhs = new_block_node(env);
         for_node->conds[0] = stmt();
         Env *for_previous_node = env;
         env = for_node->rhs->env;
         // expect_token(';');
         // TODO: to allow without lines
         if (!consume_token(';')) {
            for_node->conds[1] = assign();
            expect_token(';');
         }
         if (!consume_token(')')) {
            for_node->conds[2] = assign();
            expect_token(')');
         }
         env = for_previous_node;
         program(for_node->rhs);
         vec_push(args, (Token *)for_node);
         continue;
      }

      if (consume_token(TK_CASE)) {
         vec_push(args, (Token *)new_node(ND_CASE, node_term(), NULL));
         expect_token(':');
         continue;
      }
      if (consume_token(TK_DEFAULT)) {
         vec_push(args, (Token *)new_node(ND_DEFAULT, NULL, NULL));
         expect_token(':');
         continue;
      }

      if (consume_token(TK_SWITCH)) {
         expect_token('(');
         Node *switch_node = new_node(ND_SWITCH, node_mathexpr(), NULL);
         expect_token(')');
         switch_node->rhs = new_block_node(env);
         program(switch_node->rhs);
         vec_push(args, (Token *)switch_node);
         continue;
      }

      if ((lang & 1) && consume_token(TK_TRY)) {
         Node *node = new_node(ND_TRY, NULL, NULL);
         node->lhs = new_block_node(env);
         program(node->lhs);
         expect_token(TK_CATCH);
         node->rhs = new_block_node(env);
         program(node->rhs);
         vec_push(args, (Token *)node);
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
   if (!old_type) {
      return NULL;
   }
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
   node->sizeof_type = old_node->sizeof_type;
   node->local_variable = old_node->local_variable;
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
   type->ptrof = old_type->ptrof;
   type->ret = old_type->ret;
   // SHOULD copy, but it's useful for debugging
   // type->argc = old_type->argc;
   // type->args[j] = old_type->args[j];
   type->array_size = old_type->array_size;
   // type->initval = old_type->initval;
   type->offset = old_type->offset;
   type->is_const = old_type->is_const;
   type->is_static = old_type->is_static;
   // type->is_omiited = old_type->is_omiited;

   // type->var_name = old_type->var_name;
   if (old_type->type_name) {
      type->type_name = strdup(old_type->type_name);
   }
   type->memaccess = old_type->memaccess;
   // type->context = old_type->context;
   type->local_typedb =
       old_type->local_typedb; // TODO is duplicate of local_typedb required?
   type->base_class = old_type->base_class;
   type->is_extern = old_type->is_extern;
   return type;
}

Type *find_typed_db_without_copy(char *input, Map *db) {
   int j;
   if (!input) {
      error("Error: find_typed_db with null input\n");
   }
   for (j = 0; j < db->keys->len; j++) {
      // for struct
      if (strcmp(input, (char *)db->keys->data[j]) == 0) {
         // copy type
         Type *old_type = (Type *)db->vals->data[j];
         return old_type;
      }
   }
   return NULL;
}

Type *find_typed_db(char *input, Map *db) {
   return duplicate_type(find_typed_db_without_copy(input, db));
}

void generate_structure(Map *db) {
   if (!db) {
      return;
   }
   int size = 0;
   int offset = 0;
   int j;
   int k;

   // This depend on data structure: should be sorted as the number
   // Should skip TEMPLATE_TYPE
   // How to detect template_based class? -> local_typedb
   for (j = 0; j < db->keys->len; j++) {
      Type *structuretype = (Type *)db->vals->data[j];
      if (structuretype->ty != TY_STRUCT || structuretype->offset > 0 ||
          (structuretype->local_typedb &&
           structuretype->local_typedb->keys->len > 0)) {
         // structuretype->offset is required to treat FILE (already known)
         continue;
      }
      offset = 0;

      for (k = 0; k < structuretype->structure->keys->len; k++) {
         Type *type = (Type *)structuretype->structure->vals->data[k];
         size = type2size3(type);
         if (size > 0 && (offset % size != 0)) {
            offset += (size - offset % size);
         }
         type->offset = offset;
         offset += cnt_size(type);
      }
      structuretype->offset = offset;
   }
}

// return last templated parameter list
Type *read_template_parameter_list(Map *local_typedb) {
   Type *type = NULL;
   char *template_typename = NULL;
   expect_token(TK_TEMPLATE);
   expect_token('<');
   while (1) {
      expect_token_or_token(TK_TYPENAME, TY_CLASS);
      template_typename = expect_ident();
      type = new_type();
      type->ty = TY_TEMPLATE;
      type->type_name = template_typename;
      map_put(local_typedb, template_typename, type);
      if (!consume_token(',')) {
         break;
      }
   }
   expect_token('>');

   return type;
}

Type *read_fundamental_type(Map *local_typedb) {
   int is_const = 0;
   int is_static = 0;
   int is_extern = 0;
   Type *type = NULL;

   while (1) {
      if (consume_token(TK_EXTERN)) {
         is_extern = 1;
      } else if (tokens->data[pos]->ty == TK_STATIC) {
         is_static = 1;
         expect_token(TK_STATIC);
      } else if (tokens->data[pos]->ty == TK_CONST) {
         is_const = 1;
         expect_token(TK_CONST);
      } else if ((lang & 1) && confirm_token(TK_TEMPLATE)) {
         type = read_template_parameter_list(local_typedb);
         type->local_typedb = local_typedb;
         break;
      } else {
         break;
      }
   }

   if (tokens->data[pos]->ty == TK_ENUM) {
      // treat as anonymous enum
      expect_token(TK_ENUM);
      define_enum(0);
      type = find_typed_db("int", typedb);
   } else if (consume_token(TK_STRUCT)) {
      char *struct_name = NULL;
      if (confirm_token(TK_IDENT)) {
         struct_name = expect_ident();
      }
      if (confirm_token('{')) {
         type = struct_declartion(struct_name);
      } else {
         type = find_typed_db(struct_name, struct_typedb);
      }
   } else if ((lang & 1) && consume_token(TK_CLASS)) {
      return class_declaration(local_typedb);
   } else if (consume_token(TK_DECLTYPE)) {
      // find type and get the true value
      // 7.1.6.2(4)
      expect_token('(');
      Node *node = assign();
      expect_token(')');
      node = optimizing(analyzing(node));
      // support only when rvalue
      return duplicate_type(node->type);
   } else {
      char *ident = expect_ident();
      // looking up template parameters
      if (local_typedb && local_typedb->keys->len > 0) {
         type = find_typed_db(ident, local_typedb);
         if (!type) {
            type = find_typed_db(ident, typedb);
         }
      } else {
         type = find_typed_db(ident, typedb);
      }
      if (confirm_token('<')) {
         // by looking up template arguments
         Vector *template_types = read_template_argument_list(NULL);
         char *buf = malloc(sizeof(char) * 1024);
         Type *template_type = NULL;
         Map *template_type_db = new_map();

         {
            int j;
            sprintf(buf, "%s", type->type_name);
            for (j = 0; j < type->local_typedb->keys->len; j++) {
               template_type = (Type *)type->local_typedb->vals->data[j];
               map_put(template_type_db, template_type->type_name,
                       template_types->data[j]);
               strncat(buf, "_Template_", 12);
               strncat(buf, type2name((Type *)template_types->data[j]), 128);
            }
         }
         // create mangled template class name
         // check if already has mangled definition?
         template_type = find_typed_db(buf, typedb);
         if (template_type) {
            type = template_type;
         } else {
            type = generate_class_template(type, template_type_db);
            type->local_typedb =
                NULL; // for the sake of configuring generate_structure
            type->type_name = buf;
            map_put(typedb, buf, type);
         }
      }
   }
   if (type) {
      type->is_const = is_const;
      type->is_static = is_static;
      type->is_extern = is_extern;
   }
   return type;
}

int split_type_caller(void) {
   int tos = pos; // for skipping tk_static
   int j;
   // static may be ident or func
   if (tokens->data[tos]->ty == TK_STATIC) {
      tos++;
   }
   if (tokens->data[tos]->ty == TK_CONST) {
      tos++;
   }
   if (tokens->data[tos]->ty == TK_EXTERN) {
      tos++;
   }
   if (tokens->data[tos]->ty == TK_TEMPLATE) {
      return 3;
   }
   if (tokens->data[tos]->ty == TK_STRUCT) {
      return 3;
   }
   if (tokens->data[tos]->ty == TK_ENUM) {
      return 3;
   }
   if (tokens->data[tos]->ty == TK_DECLTYPE) {
      return 3;
   }
   if (tokens->data[tos]->ty != TK_IDENT) {
      return 0;
   }
   for (j = 0; j < typedb->keys->len; j++) {
      // for struct
      if (strcmp(tokens->data[tos]->input, (char *)typedb->keys->data[j]) ==
          0) {
         return typedb->vals->data[j]->ty;
      }
   }
   for (j = 0; current_local_typedb && j < current_local_typedb->keys->len;
        j++) {
      // for template
      if (strcmp(tokens->data[tos]->input,
                 (char *)current_local_typedb->keys->data[j]) == 0) {
         return 3;
      }
   }
   return 2; // IDENT
}

int confirm_type(void) {
   if (split_type_caller() > 2) {
      return 1;
   }
   return 0;
}
int confirm_ident(void) {
   if (split_type_caller() == 2) {
      return 1;
   }
   return 0;
}

int consume_ident(void) {
   if (split_type_caller() == 2) {
      pos++;
      return 1;
   }
   return 0;
}

char *expect_ident(void) {
   if (tokens->data[pos]->ty != TK_IDENT) {
      error("Error: Expected Ident but actual %d\n", tokens->data[pos]->ty);
      return NULL;
   }
   return tokens->data[pos++]->input;
}

void define_enum(int assign_name) {
   // ENUM def.
   consume_ident(); // for ease
   expect_token('{');
   Type *enumtype = new_type();
   enumtype->ty = TY_INT;
   enumtype->offset = 4;
   int cnt = 0;
   while (!consume_token('}')) {
      char *itemname = expect_ident();
      Node *itemnode = NULL;
      if (consume_token('=')) {
         itemnode = new_num_node_from_token(tokens->data[pos]);
         cnt = tokens->data[pos]->num_val;
         expect_token(TK_NUM);
      } else {
         itemnode = new_num_node(cnt);
      }
      cnt++;
      map_put(consts, itemname, itemnode);
      if (!consume_token(',')) {
         expect_token('}');
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
   newfunc->env->current_func = newfunc->type;
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
      thistype->var_name = "this";
      type->args[5] = type->args[4];
      type->args[4] = type->args[3];
      type->args[3] = type->args[2];
      type->args[2] = type->args[1];
      type->args[1] = type->args[0];
      type->args[0] = thistype;
      type->argc++;

      type->context->is_previous_class = class_name;
      type->context->method_name = name;
      newfunc->env->current_class = thistype;
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
   }
   newfunc->argc = type->argc;
   int i;
   int stack_arguments_ptr = 8;
   for (i = 0; i < newfunc->argc; i++) {
      newfunc->args[i] = new_ident_node_with_new_variable(
          type->args[i]->var_name, type->args[i]);
      if (type->args[i]->ty == TY_STRUCT) {
         // Use STACK Based pointer
         newfunc->args[i]->local_variable->lvar_offset = -stack_arguments_ptr;
      }
   }
   env = prev_env;
   // to support prototype def.
   if (confirm_token('{')) {
      current_local_typedb = local_typedb;
      program(newfunc);
      current_local_typedb = NULL;
      // newfunc->is_recursive = is_recursive(newfunc, name);
      if (local_typedb->keys->len <= 0) {
         // There are no typedb: template
         vec_push(globalcode, (Token *)newfunc);
      }
   } else {
      expect_token(';');
   }
}

Type *class_declaration(Map *local_typedb) {
   int j;
   Type *structuretype = new_type();
   structuretype->structure = new_map();
   structuretype->ty = TY_STRUCT;
   structuretype->ptrof = NULL;
   char *structurename = expect_ident();
   structuretype->type_name = structurename;
   if (consume_token(':')) {
      // inheritance
      char *base_class_name = expect_ident();
      structuretype->base_class = find_typed_db(base_class_name, typedb);
      if (!structuretype->base_class ||
          structuretype->base_class->ty != TY_STRUCT) {
         error("Error: Inherited Class Not Found %s\n", base_class_name);
      }
      for (j = 0; j < structuretype->base_class->structure->keys->len; j++) {
         // copy into new class
         Type *type = duplicate_type(
             (Type *)structuretype->base_class->structure->vals->data[j]);
         if (type->memaccess == PRIVATE) {
            // inherited from private cannot seen anymore
            type->memaccess = HIDED;
         }
         map_put(structuretype->structure,
                 (char *)structuretype->base_class->structure->keys->data[j],
                 (Node *)type);
         // On Function, We should renewal its declartion
      }
   }
   expect_token('{');
   MemberAccess memaccess;
   memaccess = PRIVATE; // on Struct, it is public
   while (!consume_token('}')) {
      if (consume_token(TK_PUBLIC)) {
         memaccess = PUBLIC;
         expect_token(':');
         continue;
      }
      if (consume_token(TK_PROTECTED)) {
         memaccess = PROTECTED;
         expect_token(':');
         continue;
      }
      if (consume_token(TK_PRIVATE)) {
         memaccess = PRIVATE;
         expect_token(':');
         continue;
      }

      char *name = NULL;
      Type *type = NULL;
      if (confirm_token(TK_IDENT) &&
          strcmp(tokens->data[pos]->input, structurename) == 0) {
         // treat as constructor
         type = find_typed_db(
             "void", typedb); // TODO 本来 Constructor はvoid型を返すのではない
      } else {
         type = read_fundamental_type(local_typedb);
      }
      type = read_type(type, &name, local_typedb);

      // TODO it should be integrated into new_fdef_node()'s "this"
      if ((lang & 1) && strstr(name, "::") && type->ty == TY_FUNC &&
          type->is_static == 0) {
         type->context->is_previous_class = structurename;
      }
      expect_token(';');
      type->memaccess = memaccess;
      map_put(structuretype->structure, name, type);
   }
   // to set up local_typedb after instanciate
   structuretype->local_typedb = local_typedb;
   map_put(typedb, structurename, structuretype);
   return structuretype;
}

Type *struct_declartion(char* struct_name) {
   // struct_name : on struct name
   Type *structuretype = new_type();
   if (struct_name) {
      map_put(struct_typedb, struct_name, structuretype);
   }
   expect_token('{');
   structuretype->structure = new_map();
   structuretype->ty = TY_STRUCT;
   structuretype->ptrof = NULL;
   while (!consume_token('}')) {
      char *name = NULL;
      Type *type = read_type_all(&name);
      type->memaccess = PUBLIC;
      expect_token(';');
      type->type_name = name;
      map_put(structuretype->structure, name, type);
   }
   structuretype->type_name = struct_name;
   // structuretype->offset = offset;
   /*
   char *name = expect_ident();
   expect_token(';');
   map_put(typedb, name, structuretype);
   */
   return structuretype;
}

void toplevel(void) {
   /*
    * C:
    * translation-unit:
         external-declaration
         translation-unit external-declaration
      external-declaration:
         function-definition
         declaration

      C++:
      toplevel (
      declaration-seq:
         declaration
         declaration-seq declaration
      declaration:
         block-declaration
         function-definition
         template-declaration
         explicit-instantiation
         explicit-specialization
         linkage-specification
         namespace-definition
         empty-declaration
         attribute-declaration
      */
   strcpy(arg_registers[0], "rdi");
   strcpy(arg_registers[1], "rsi");
   strcpy(arg_registers[2], "rdx");
   strcpy(arg_registers[3], "rcx");
   strcpy(arg_registers[4], "r8");
   strcpy(arg_registers[5], "r9");

   // consume_token('{')
   // idents = new_map...
   // stmt....
   // consume_token('}')
   global_vars = new_map();
   funcdefs = new_map();
   funcdefs_generated_template = new_map();
   consts = new_map();
   strs = new_vector();
   floats = new_vector();
   float_doubles = new_vector();
   globalcode = new_vector();
   env = NULL;

   while (!consume_token(TK_EOF)) {
      if (consume_token(TK_TYPEDEF)) {
         if (consume_token(TK_ENUM)) {
            // not anonymous enum
            define_enum(1);
            expect_token(';');
            continue;
         }
         // struct-or-union-specifier:
         // (in this if) struct-or-union identifier opt {
         // struct-declaration-list } (general typedef) struct-or-union
         // identifier
         /*
         if (confirm_token(TK_STRUCT) && (tokens->data[pos + 1]->ty == '{' ||
                                          tokens->data[pos + 2]->ty == '{')) {
            expect_token(TK_STRUCT);
            struct_declartion();
            continue;
         }*/

         char *input = NULL;
         Type *type = read_fundamental_type(NULL);
         type = read_type(type, &input, NULL);
         type->type_name = input;
         map_put(typedb, input, type);
         expect_token(';');
         continue;
      }

      if (consume_token(TK_ENUM)) {
         consume_ident(); // for ease
         expect_token('{');
         char *name = expect_ident();
         Type *structuretype = new_type();
         expect_token(';');
         map_put(typedb, name, structuretype);
         continue;
      }

      if (confirm_token('{')) {
         Node *newblocknode = new_block_node(NULL);
         program(newblocknode);
         vec_push(globalcode, (Token *)newblocknode);
         continue;
      }

      // Constructor
      if ((lang & 1) && confirm_token(TK_IDENT) &&
          strstr(tokens->data[pos]->input, "::")) {
         // C :: C のかたち
         Type *type = find_typed_db(
             "void", typedb); // TODO 本来 Constructor はvoid型を返すのではない
         char *name = NULL;
         Map *local_typedb = new_map();
         type = read_type(type, &name, local_typedb);

         // 今のところ自動的に暗黙の引数this がつく
         new_fdef(name, type, local_typedb);
         // Class に Constructor をひもつける、必要あるのか？
         continue;
      }

      char *name = NULL;
      Map *local_typedb = new_map();
      Type *type = read_fundamental_type(local_typedb);
      type = read_type(type, &name, local_typedb);
      if (!name) {
         // there are no names -> anomymous -> empty declaration
         expect_token(';');
         continue;
      }
      if (type->ty == TY_FUNC) {
         new_fdef(name, type, local_typedb);
         continue;
      } else {
         // Global Variables.
         map_put(global_vars, name, type);
         type->initval = 0;
         if (consume_token('=')) {
            Node *initval = assign();
            initval = optimizing(analyzing(initval));
            switch (initval->ty) {
               case ND_NUM:
                  type->initval = initval->num_val;
                  break;
               case ND_STRING:
                  // TODO: only supported pointer-based type.
                  type->initstr = initval->name;
                  break;
               default:
                  error("Error: invalid initializer on global variable\n");
                  break;
            }
         }
         expect_token(';');
      }
      continue;
   }
   vec_push(globalcode, (Token *)new_block_node(NULL));
}

Type *generate_class_template(Type *type, Map *template_type_db) {
   type = duplicate_type(type);
   Type *new_type;
   int j;

   if (type && type->ty == TY_TEMPLATE) {
      new_type = find_typed_db(type->type_name, template_type_db);
      if (!new_type) {
         error("Error: Incorrect Class Template type: %s\n", type->type_name);
      }
      type = duplicate_type(new_type);
   }
   if (type->ptrof) {
      type->ptrof = generate_class_template(type->ptrof, template_type_db);
   }
   if (type->ret) {
      type->ret = generate_class_template(type->ret, template_type_db);
   }
   if (type->argc > 0) {
      for (j = 0; j < type->argc; j++) {
         if (type->args[j]) {
            type->args[j] =
                generate_class_template(type->args[j], template_type_db);
         }
      }
   }
   if (type->structure) {
      Map *new_structure = new_map();
      for (j = 0; j < type->structure->keys->len; j++) {
         if (type->structure->vals->data[j]) {
            map_put(
                new_structure, (char *)type->structure->keys->data[j],
                (Token *)generate_class_template(
                    (Type *)type->structure->vals->data[j], template_type_db));
         }
      }
      type->structure = new_structure;
   }
   return type;
}

Node *generate_template(Node *node, Map *template_type_db) {
   node = duplicate_node(node);
   int j;
   Node *code_node;
   Type *new_type;
   Env *prev_env = env;
   Env *duplicated_env = NULL;
   LocalVariable *lvar;
   if (node->ty == ND_BLOCK || node->ty == ND_FDEF) {
      duplicated_env = new_env(node->env->env, NULL);
      env = duplicated_env;
      for (j = 0; j < node->env->idents->keys->len; j++) {
         lvar = (LocalVariable *)node->env->idents->vals->data[j];
         new_local_variable(lvar->name, lvar->type);
      }
   }
   if (node->ty == ND_FDEF) {
      // apply to node->type->ret if TY_TEMPLATE
      if (node->type->ret->ty == TY_TEMPLATE) {
         new_type = find_typed_db(node->type->ret->type_name, template_type_db);
         if (!new_type) {
            error("Error: Incorrect Template type: %s\n",
                  node->type->ret->type_name);
         }
         node->type->ret = copy_type(new_type, new_type);
         // copy to env
         env->ret = node->type->ret;
      }
   }
   if (node->type && node->type->ty == TY_TEMPLATE) {
      new_type = find_typed_db(node->type->type_name, template_type_db);
      if (!new_type) {
         error("Error: Incorrect Template type: %s\n", node->type->type_name);
      }
      node->type = copy_type(new_type, new_type);
   }
   if (node->lhs) {
      node->lhs = generate_template(node->lhs, template_type_db);
   }
   if (node->rhs) {
      node->rhs = generate_template(node->rhs, template_type_db);
   }

   if (node->argc > 0) {
      for (j = 0; j < node->argc; j++) {
         if (node->args[j]) {
            node->args[j] = generate_template(node->args[j], template_type_db);
         }
      }
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

   for (j = 0; j < 3; j++) {
      if (node->conds[j]) {
         node->conds[j] = generate_template(node->conds[j], template_type_db);
      }
   }
   env = prev_env;
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

void globalvar_gen(void) {
   puts(".data");
   int j;
   for (j = 0; j < global_vars->keys->len; j++) {
      Type *valdataj = (Type *)global_vars->vals->data[j];
      char *keydataj = (char *)global_vars->keys->data[j];
      if (valdataj->is_extern) {
         // Do not write externed values.
         continue;
      }
      if (valdataj->initstr) {
         printf(".type %s,@object\n", keydataj);
         printf(".global %s\n", keydataj);
         printf(".size %s, %d\n", keydataj, cnt_size(valdataj));
         printf("%s:\n", keydataj);
         printf(".quad %s\n", valdataj->initstr);
         puts(".text");
      } else if (valdataj->initval == 0) {
         printf(".type %s,@object\n", keydataj);
         printf(".global %s\n", keydataj);
         printf(".comm %s, %d\n", keydataj, cnt_size(valdataj));
      } else {
         printf(".type %s,@object\n", keydataj);
         printf(".global %s\n", keydataj);
         printf(".size %s, %d\n", keydataj, cnt_size(valdataj));
         printf("%s:\n", keydataj);
         printf(".long %d\n", valdataj->initval);
         puts(".text");
      }
   }
   for (j = 0; j < strs->len; j++) {
      printf(".LC%d:\n", j);
      printf(".string \"%s\"\n", (char *)strs->data[j]);
   }
   for (j = 0; j < floats->len; j++) {
      printf(".LCF%d:\n", j);
      int *int_repr = (int *)floats->data[j];
      printf(".long %d\n", *int_repr);
   }
   for (j = 0; j < float_doubles->len; j++) {
      printf(".LCD%d:\n", j);
      long *repr = (long *)float_doubles->data[j];
      printf(".long %ld\n", *repr & 0xFFFFFFFF);
      printf(".long %ld\n", *repr >> 32);
   }

   // using in ND_NEG
   if (neg_double_generate) {
      printf(".LCDNEGDOUBLE:\n");
      printf(".long 0\n");
      printf(".long -2147483648\n");
   }
   if (neg_float_generate) {
      printf(".LCDNEGFLOAT:\n");
      printf(".long 2147483648\n");
   }
}

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

void init_typedb(void) {
   struct_typedb = new_map();
   typedb = new_map();
   current_local_typedb = NULL; // Initiazlied on function.

   Type *type_auto = new_type();
   type_auto->ty = TY_AUTO;
   type_auto->ptrof = NULL;
   type_auto->type_name = "auto";
   map_put(typedb, "auto", type_auto);

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

   Type *va_listarray = new_type();
   Type *va_listtype = new_type();
   va_listarray->type_name = "va_list";
   va_listarray->ty = TY_ARRAY;
   va_listarray->array_size = 1;
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
   va_listarray->ptrof = va_listtype;
   map_put(typedb, "va_list", va_listarray);

   Type *typedou = new_type();
   typedou->ty = TY_FLOAT;
   typedou->ptrof = NULL;
   typedou->offset = 4;
   map_put(typedb, "float", typedou);

   typedou = new_type();
   typedou->ty = TY_DOUBLE;
   typedou->ptrof = NULL;
   typedou->offset = 8;
   map_put(typedb, "double", typedou);
}

Vector *read_tokenize(char *fname) {
   FILE *fp = fopen(fname, "r");
   if (!fp) {
      error("No file found: %s\n", fname);
   }
   fseek(fp, 0, SEEK_END);
   long length = ftell(fp);
   fseek(fp, 0, SEEK_SET);
   char *buf = malloc(sizeof(char) * (length + 5));
   fread(buf, length + 5, sizeof(char), fp);
   fclose(fp);
   return tokenize(buf);
}

Node *implicit_althemic_type_conversion(Node *node) {
   // See Section 6.3.1.8
   if (node->lhs->type->ty == node->rhs->type->ty) {
      return node;
   }
   if (node->lhs->type->ty == TY_DOUBLE) {
      node->rhs = new_node(ND_CAST, node->rhs, NULL);
      node->rhs->type = node->lhs->type;
      return node;
   }
   if (node->rhs->type->ty == TY_DOUBLE) {
      node->lhs = new_node(ND_CAST, node->lhs, NULL);
      node->lhs->type = node->rhs->type;
      return node;
   }
   if (node->lhs->type->ty == TY_FLOAT) {
      node->rhs = new_node(ND_CAST, node->rhs, NULL);
      node->rhs->type = node->lhs->type;
      return node;
   }
   if (node->rhs->type->ty == TY_FLOAT) {
      node->lhs = new_node(ND_CAST, node->lhs, NULL);
      node->lhs->type = node->rhs->type;
      return node;
   }
   if (type2size(node->lhs->type) < type2size(node->rhs->type)) {
      node->lhs = new_node(ND_CAST, node->lhs, NULL);
      node->lhs->type = node->rhs->type;
      return node;
   }
   if (type2size(node->lhs->type) > type2size(node->rhs->type)) {
      node->rhs = new_node(ND_CAST, node->rhs, NULL);
      node->rhs->type = node->lhs->type;
      return node;
   }
   return node;
}

LocalVariable *new_local_variable(char *name, Type *type) {
   LocalVariable *lvar = malloc(sizeof(LocalVariable));
   lvar->name = name;
   lvar->type = type;
   lvar->lvar_offset = 0;
   map_put(env->idents, name, lvar);
   return lvar;
}

Node *analyzing(Node *node) {
   int j;
   Env *prev_env = env;
   if (node->ty == ND_BLOCK || node->ty == ND_FDEF) {
      env = node->env;
      LocalVariable *local_variable = NULL;
      // generate all size of nodes
      // preview all variables and setup offsets.
      if (prev_env) {
         // This is to update rsp_offset based on (it's based system).
         // Previously, this is called by new_env() but no longer supported.
         env->rsp_offset += prev_env->rsp_offset;
      }
      for (j = 0; j < env->idents->keys->len; j++) {
         local_variable = (LocalVariable *)env->idents->vals->data[j];
         if (local_variable->lvar_offset != 0) {
            // For Example: STRUCT Passed Argumenets (use rsp+24 or so on)
            continue;
         }
         int size = cnt_size(local_variable->type);
         // should aligned as x86_64
         if (size % 8 != 0) {
            size += (8 - size % 8);
         }
         update_rsp_offset(size);
         local_variable->lvar_offset = env->rsp_offset;
      }
   }

   if (node->lhs) {
      node->lhs = analyzing(node->lhs);
   }
   if (node->rhs) {
      node->rhs = analyzing(node->rhs);
   }
   if (node->is_omiited) {
      node->is_omiited = analyzing(node->is_omiited);
   }
   for (j = 0; j < 3; j++) {
      if (node->conds[j]) {
         node->conds[j] = analyzing(node->conds[j]);
      }
   }
   if (node->argc > 0) {
      for (j = 0; j < node->argc; j++) {
         if (node->args[j]) {
            node->args[j] = analyzing(node->args[j]);
         }
      }
   }
   if (node->code) {
      for (j = 0; j < node->code->len && node->code->data[j]; j++) {
         node->code->data[j] = (Token *)analyzing((Node *)node->code->data[j]);
      }
   }

   switch (node->ty) {
      case ND_RETURN:
         if (env->ret == NULL) {
            error("try to Return to NULL: Unexpected behaviour\n");
         }

         // treat TY_AUTO
         if (env->ret->ty == TY_AUTO) {
            copy_type(node->lhs->type, env->ret);
         }
         if (node->lhs) {
            node->type = node->lhs->type;
         } // if return to VOID, there is no lhs
         break;
      case ND_SIZEOF:
         if (node->conds[0]) {
            // evaluate node->conds[0] and return its type
            node = new_long_num_node(cnt_size(node->conds[0]->type));

         } else {
            // ND_SIZEOF should generate its type information.
            // This is because: on parse, no (structured) type size are
            // avaliable
            node = new_long_num_node(cnt_size(node->sizeof_type));
         }
         break;
      case ND_ADD:
      case ND_SUB:
         if (node->lhs->type->ty == TY_PTR || node->lhs->type->ty == TY_ARRAY) {
            node->rhs = new_node(ND_MULTIPLY_IMMUTABLE_VALUE, node->rhs, NULL);
            node->rhs->type = node->rhs->lhs->type;
            node->rhs->num_val = cnt_size(node->lhs->type->ptrof);
            node->type = node->lhs->type;
         } else if (node->rhs->type->ty == TY_PTR ||
                    node->rhs->type->ty == TY_ARRAY) {
            node->lhs = new_node(ND_MULTIPLY_IMMUTABLE_VALUE, node->lhs, NULL);
            node->lhs->type = node->rhs->lhs->type;
            node->lhs->num_val = cnt_size(node->rhs->type->ptrof);
            node->type = node->rhs->type;
         } else {
            node = implicit_althemic_type_conversion(node);
            node->type = node->lhs->type;
            break;
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
      case ND_MUL:
         node = implicit_althemic_type_conversion(node);
         node->type = node->lhs->type;
         break;
      case ND_ISEQ:
      case ND_ISNOTEQ:
      case ND_ISLESSEQ:
      case ND_ISMOREEQ:
      case ND_LESS:
      case ND_GREATER:
         node->type = find_typed_db("int", typedb);
         node = implicit_althemic_type_conversion(node);
         break;
      case ND_FPLUSPLUS:
      case ND_FSUBSUB:
         // Moved to analyzing process.
         if (node->lhs->type->ty == TY_PTR || node->lhs->type->ty == TY_ARRAY) {
            node->num_val = type2size(node->lhs->type->ptrof);
         }
         node->type = node->lhs->type;
         break;
      case ND_ADDRESS:
         node->type = new_type();
         node->type->ty = TY_PTR;
         node->type->ptrof = node->lhs->type;
         break;
      case ND_DEREF:
         node->type = node->lhs->type->ptrof;
         if (!node->type) {
            error("Error: Dereference on NOT pointered.");
         }
         break;
      case ND_ASSIGN:
         if (node->lhs->type->is_const) {
            error("Error: Assign to constant value.\n");
         }
         if (node->lhs->type->ty != node->rhs->type->ty) {
            node->rhs = new_node(ND_CAST, node->rhs, NULL);
            node->rhs->type = node->lhs->type;
         }
         node->type = node->lhs->type;
         break;
      case ND_DOT:
         if (node->lhs->type->ty != TY_STRUCT) {
            error("Error: dot operator to NOT struct\n");
         }
         node->type = (Type *)map_get(node->lhs->type->structure, node->name);
         if (!node->type) {
            error("Error: structure not found: %s\n", node->name);
         }
         // Member Accesss Control p.231
         if ((lang & 1) && (node->type->memaccess == HIDED)) {
            error("Error: access to private item: %s\n", node->name);
         }
         if ((lang & 1) && ((node->type->memaccess == PRIVATE) ||
                            (node->type->memaccess == PROTECTED))) {
            if (!env->current_class || !env->current_class->var_name ||
                (strcmp(env->current_class->var_name, "this"))) {
               // TODO
               //(strcmp(env->current_class->type_name,
               // node->type->type_name)))) {
               error("Error: access to private item: %s\n", node->name);
            }
         }
         break;
      case ND_NEG:
         node->type = node->lhs->type;
         if (node->type->ty == TY_DOUBLE) {
            neg_double_generate = 1;
         }
         if (node->type->ty == TY_FLOAT) {
            neg_float_generate = 1;
         }
         break;
      case ND_MULTIPLY_IMMUTABLE_VALUE:
         node->type = node->lhs->type;
         break;
      case ND_COMMA:
         node->type = node->rhs->type;
         break;
      case ND_FUNC:
         if (node->funcdef) {
            // there are new typedefs. {
            node->type = node->funcdef->type->ret;
         } else {
            node->type = find_typed_db("int", typedb);
         }
         break;
      case ND_VASTART:
         node->num_val = env->current_func->argc;
         node->type = node->lhs->type;
         break;
      default:
         if (node->type == NULL && node->lhs && node->lhs->type) {
            node->type = node->lhs->type;
         }
         break;
   }

   env = prev_env;
   return node;
}

Node *optimizing(Node *node) {
   Node *node_new;
   int new_num_val;
   int j;

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
   switch (node->ty) {
      case ND_NEG:
         if (node->lhs->ty == ND_NUM) {
            node = node->lhs;
            node->num_val = -node->num_val;
         }
         break;

      case ND_ADD:
      case ND_SUB:
      case ND_MUL:
      case ND_DIV:
      case ND_MOD:
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
               case ND_DIV:
                  new_num_val = node->lhs->num_val / node->rhs->num_val;
                  break;
               case ND_MOD:
                  new_num_val = node->lhs->num_val % node->rhs->num_val;
                  break;
               default:
                  error("Error: Unsupport type in optimization: %d\n",
                        node->ty);
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
         if ((node->funcdef->code->len > 0) && (node->funcdef->code->len < 2) &&
             (node->funcdef->is_recursive == 0) &&
             (node->type->ty == TY_VOID)) {

            node_new = new_block_node(NULL);
            for (j = 0; j < node->argc; j++) {
               // 掘り下げた後の offset にあるように Env を作りなおす
               Node *copied_node = duplicate_node(node->funcdef->args[j]);
               /*
                * LocalVariable *lvar = duplicate_lvar(copied_node->lvar);
               lvar->offset += node->funcdef->env->rsp_offset_max;
               copied_node->lvar = lvar;
               */
               node_new->args[j] =
                   new_node(ND_ASSIGN, copied_node, node->args[j]);
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
         break;
      case ND_MULTIPLY_IMMUTABLE_VALUE:
         if (node->lhs->ty == ND_NUM) {
            /*
             mov r12d, 1
             imul r12d, 1
             */
            // can be rewritten as
            /* mov r12d, 1 */
            node->lhs->num_val *= node->num_val;
            node = node->lhs;
            // because node and node->lhs shares same type, no need to worry
            // about type
         }
      default:
         break;
   }
   return node;
}

void analyzing_process(void) {
   generate_structure(typedb);
   generate_structure(struct_typedb);
   int len = globalcode->len;
   int i;
   for (i = 0; i < len; i++) {
      globalcode->data[i] = (Token *)analyzing((Node *)globalcode->data[i]);
   }
}

void optimizing_process(void) {
   int len = globalcode->len;
   int i;
   for (i = 0; i < len; i++) {
      globalcode->data[i] = (Token *)optimizing((Node *)globalcode->data[i]);
   }
}

int main(int argc, char **argv) {
   if (argc < 2) {
      error("Incorrect Arguments\n");
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
      } else if (strcmp(argv[i], "-h8300") == 0) {
         output |= 1;
      }
   }
   if (is_from_file) {
      preprocess(read_tokenize(argv[argc - 1]), argv[argc - 1]);
   } else {
      preprocess(tokenize(argv[argc - 1]), NULL);
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
   if (is_register && !output) {
      gen_register_top();
   }
   // for using h8300 output
   //if (is_register && output) 

   return 0;
}
