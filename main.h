// main.h
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

#ifndef __HANANDO_FUKUI_MAIN__
#define __HANANDO_FUKUI_MAIN__

typedef enum {
   TK_ADD = '+',
   TK_SUB = '-',
   TK_EQUAL = '=',
   TK_BLOCKBEGIN = '{',
   TK_BLOCKEND = '}',
   TK_S = '#',
   TK_PLUSPLUS = 86, // '+' + '+',
   TK_SUBSUB = 90, // '-' + '-',
   TK_NUM = 256,
   TK_FLOAT,
   TK_IDENT,
   TK_EOF,
   TK_ISEQ,
   TK_ISNOTEQ,
   TK_IF,
   TK_WHILE,
   TK_FOR,
   TK_ELSE,
   TK_DO,
   TK_RSHIFT,
   TK_LSHIFT,
   TK_OPAS,
   TK_GOTO,
   TK_RETURN,
   TK_SIZEOF,
   TK_OR,
   TK_AND,
   TK_STRUCT,
   TK_TYPEDEF,
   TK_STRING,
   TK_SPACE,
   TK_NEWLINE,
   TK_ISLESSEQ,
   TK_ISMOREEQ,
   TK_BREAK,
   TK_CONTINUE,
   TK_NULL,
   TK_SWITCH,
   TK_CASE,
   TK_ENUM,
   TK_ARROW,
   TK_CONST,
   TK_DEFAULT,
   TK_EXTERN,
} TokenConst;

typedef enum {
   ND_ADD = '+',
   ND_SUB = '-',
   ND_MUL = '*',
   ND_EQUAL = '=',
   ND_DIV = '/',
   ND_MOD = '%',
   ND_XOR = '^',
   ND_OR = '|',
   ND_AND = '&',
   ND_LESS = '<',
   ND_GREATER = '>',
   ND_COMMA = ',',
   ND_NUM = 256,
   ND_FLOAT,
   ND_IDENT,
   ND_GLOBAL_IDENT,
   ND_ISEQ,
   ND_ISNOTEQ,
   ND_ISLESSEQ,
   ND_ISMOREEQ,
   ND_FUNC,
   ND_FDEF,
   ND_RSHIFT,
   ND_LSHIFT,
   ND_INC,
   ND_DEC,
   ND_BLOCK,
   ND_IF,
   ND_WHILE,
   ND_DOWHILE,
   ND_FOR,
   ND_RETURN,
   ND_ADDRESS,
   ND_DEREF,
   ND_GOTO,
   ND_LOR,
   ND_LAND,
   ND_FPLUSPLUS,
   ND_FSUBSUB,
   ND_STRING,
   ND_CONTINUE,
   ND_BREAK,
   ND_NEG,
   ND_CAST,
   ND_SWITCH,
   ND_CASE,
   ND_EXTERN_SYMBOL,
   ND_DEFAULT,
} NodeType;

typedef struct {
   TokenConst ty;
   long num_val;
   char *input;
   int pos;
} Token;

typedef struct {
   Token **data;
   long capacity;
   long len;
} Vector;

typedef struct {
   Vector *keys;
   Vector *vals;
} Map;

typedef struct Type {
   enum { TY_INT = 3, TY_PTR, TY_ARRAY, TY_CHAR, TY_LONG, TY_STRUCT, TY_VOID, TY_DOUBLE} ty;
   Map *structure; // <name, Type*>
   struct Type *ptrof;
   int array_size;
   int initval;
   int offset;
} Type;

typedef struct Env {
   struct Env *env;
   Map *idents;
   int rsp_offset_all;
   int rsp_offset;
} Env;

typedef struct Node {
   NodeType ty;
   struct Node *lhs;
   struct Node *rhs;
   struct Node *args[6];
   Vector *code;
   long argc;
   long num_val;
   char *name;
   Env *env;
   Type *type;
} Node;


// Extended Register. with global variables, local variables, memory map, registers, ...
typedef struct Register {
   enum {R_REGISTER, R_LVAR, R_GVAR} kind;
   int id; // offset or type
   char* name; // for global variable
} Register;

Map *new_map();
int map_put(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);
Vector *new_vector();
int vec_push(Vector *vec, Token *element);

#endif /* __HANANDO_FUKUI_MAIN__ */
