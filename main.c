// main.c
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
#include <ctype.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPACE_R12R13R14R15 32

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
Env *env;

int is_recursive(Node *node, char *name);

int lang = 0;
int output = 0;

Node *new_func_node(Node *ident, Vector *template_types, Vector *args);
Type *read_type_all(char **input);
Type *read_type(Type *type, char **input, Map *local_typedb);
Type *struct_declartion(char* struct_name);
Type *read_fundamental_type(Map *local_typedb);
int confirm_type();
int confirm_token(TokenConst ty);
int confirm_ident();
int consume_ident();
int split_type_caller();
void define_enum(int use);
char *expect_ident();
void program(Node *block_node);
LocalVariable *get_type_local(Node *node);
LocalVariable *new_local_variable(char *name, Type *type);
Node *generate_template(Node *node, Map *template_type_db);
Type *generate_class_template(Type *type, Map *template_type_db);

Node *node_mul();
Node *node_term();
Node *node_assignment_expression();
Node *node_land();
Node *node_or();
Node *node_lor();
Node *node_and();
Node *node_xor();
Node *node_shift();
Node *node_add();
Node *node_cast();
Node *node_unary();
Node *node_expression();

Node *optimizing(Node *node);

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
   Node *node = NULL;
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
         node->conds[0] = node_unary();
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
            vec_push(args, (Token *)node_assignment_expression());
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
      node = node_expression();
      if (consume_token(')') == 0) {
         // TODO
         // fprintf(stderr, "Error: Incorrect Parensis.\n");
         return NULL;
      }
   } else if (confirm_token(TK_STRING)) {
      char *_str = malloc(sizeof(char) * 256);
      snprintf(_str, 255, ".LC%d",
               vec_push(strs, (Token *)tokens->data[pos]->input));
      node = new_string_node(_str);
      expect_token(TK_STRING);
   }

   if (!node) {
      // TODO
      //fprintf(stderr, "Error: No Primary Expression.\n");
      return NULL;
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
            vec_push(args, (Token *)node_assignment_expression());
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
         node = new_node(ND_DEREF, new_node('+', node, node_expression()), NULL);
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

Node *node_assignment_expression(void) {
   int old_pos = pos; // save old position
   Node *node;
   node = node_unary();
   if (!consume_token('=')) {
      pos = old_pos;
      // TODO: Support Conditional Expression
      //node = node_conditional_expression();
      return node;
   }

   if (confirm_token(TK_OPAS)) {
      NodeType tp = tokens->data[pos]->input[0];
      expect_token(TK_OPAS);
      if (tp == '<') {
         tp = ND_LSHIFT;
      }
      if (tp == '>') {
         tp = ND_RSHIFT;
      }
      node = new_assign_node(node, new_node_with_cast(tp, node, node_expression()));
   } else {
      node = new_assign_node(node, node_expression());
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
         node = new_node(ND_RETURN, node_expression(), NULL);
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
                     vec_push(args, (Token *)node_assignment_expression());
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
               node = new_node('=', node, node_assignment_expression());
            }
         }
         input = NULL;
      } while (consume_token(','));
   } else {
      node = node_expression();
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
   node->args[0] = node_expression();
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
         Node *while_node = new_node(ND_WHILE, node_expression(), NULL);
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
         do_node->lhs = node_expression();
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
            for_node->conds[1] = node_expression();
            expect_token(';');
         }
         if (!consume_token(')')) {
            for_node->conds[2] = node_expression();
            expect_token(')');
         }
         env = for_previous_node;
         program(for_node->rhs);
         vec_push(args, (Token *)for_node);
         continue;
      }

      if (consume_token(TK_CASE)) {
         // TODO 本来はStatement
         vec_push(args, (Token *)new_node(ND_CASE, node_expression(), NULL));
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
         Node *switch_node = new_node(ND_SWITCH, node_expression(), NULL);
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
      Node *node = node_expression();
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
            Node *initval = node_expression();
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

Node *node_expression(void) {
   Node *node = node_assignment_expression();
   while (1) {
      if (consume_token(',')) {
         node = new_node(',', node, node_assignment_expression());
      } else {
         return node;
      }
   }
   return node;
}

Node *node_conditional_expression(void) {
   Node *node = node_lor();
   Node *conditional_node = NULL;
   if (consume_token('?')) {
      conditional_node = new_node('?', node_expression(), NULL);
      conditional_node->conds[0] = node;
      expect_token(':');
      conditional_node->rhs = node_conditional_expression();
      node = conditional_node;
   }
   return node;
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

LocalVariable *new_local_variable(char *name, Type *type) {
   LocalVariable *lvar = malloc(sizeof(LocalVariable));
   lvar->name = name;
   lvar->type = type;
   lvar->lvar_offset = 0;
   map_put(env->idents, name, lvar);
   return lvar;
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
