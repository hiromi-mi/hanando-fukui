// main.h
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

#ifndef __HANANDO_FUKUI_MAIN__
#define __HANANDO_FUKUI_MAIN__

typedef enum {
   TK_ADD = '+',
   TK_SUB = '-',
   TK_EQUAL = '=',
   TK_BLOCKBEGIN = '{',
   TK_BLOCKEND = '}',
   TK_QUESTION = '?',
   TK_S = '#',
   TK_PLUSPLUS = 86, // '+' + '+',
   TK_SUBSUB = 90,   // '-' + '-',
   TK_NUM = 256,
   TK_FLOAT,
   TK_DOUBLE,
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
   TK_CLASS,
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
   TK_OMIITED, // ...
   TK_PUBLIC,
   TK_PRIVATE,
   TK_STATIC,
   TK_PROTECTED,
   TK_COLONCOLON,
   TK_TEMPLATE,
   TK_TYPENAME,
   TK_TRY,
   TK_CATCH,
   TK_THROW,
   TK_DECLTYPE,
   TK_NEW,
} TokenConst;

typedef enum {
   ND_ADD = '+',
   ND_SUB = '-',
   ND_MUL = '*',
   ND_ASSIGN = '=',
   ND_DIV = '/',
   ND_MOD = '%',
   ND_XOR = '^',
   ND_OR = '|',
   ND_AND = '&',
   ND_DOT = '.',
   ND_LESS = '<',
   ND_GREATER = '>',
   ND_COMMA = ',',
   ND_APOS = '!',
   ND_QUESTION = '?',
   ND_NUM = 256,
   ND_MULTIPLY_IMMUTABLE_VALUE,
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
   ND_VAARG,
   ND_VASTART,
   ND_SYMBOL,
   ND_VAEND,
   ND_EXPRESSION_BLOCK,
   ND_SIZEOF,
   ND_TRY,
   ND_THROW,
} NodeType;

typedef struct {
   TokenConst ty;
   long num_val;
   double float_val;
   char *input;
   int pos;
   int type_size; // to treat 'a' as char. only used in TK_NUM
   int pline;
} Token;

typedef struct {
   Token **data;
   long capacity;
   long len;
} Vector;

typedef enum {
   PRIVATE,
   PUBLIC,
   HIDED,
   PROTECTED,
} MemberAccess;

typedef struct {
   Vector *keys;
   Vector *vals;
} Map;

typedef struct Context {
   char *is_previous_class;
   char *method_name;
} Context;

typedef struct Type {
   enum {
      TY_INT = 3,
      TY_PTR,
      TY_ARRAY,
      TY_CHAR,
      TY_LONG,
      TY_STRUCT,
      TY_CLASS,
      TY_VOID,
      TY_FLOAT,
      TY_DOUBLE,
      TY_FUNC,
      TY_TEMPLATE,
      TY_AUTO,
   } ty;
   Map *structure; // <name, Type*>
   struct Type *ptrof;
   struct Type *ret;
   int argc;
   struct Type *args[6];
   int array_size;
   int offset;
   int is_const;
   int is_static;
   int is_omiited;
   int is_extern;
   char *var_name; // for args
   char *type_name;
   MemberAccess memaccess;
   Context *context;
   Map *local_typedb;
   struct Type *base_class;
} Type;

typedef struct Env {
   struct Env *env;
   Map *idents;
   int *rsp_offset_max;
   int rsp_offset;
   Type *ret;
   Type *current_class;
   Type *current_func;
} Env;

typedef struct LocalVariable {
   int lvar_offset;
   Type *type;
   char *name;
} LocalVariable;

typedef struct GlobalVariable {
   Vector *inits; // Node*
   Type *type;
   char *name;
} GlobalVariable;

typedef struct Node {
   NodeType ty;
   struct Node *lhs;
   struct Node *rhs;
   struct Node *conds[3];
   struct Node *args[6];
   Vector *code;
   long argc;
   long num_val;
   double float_val;
   char *name;     // Name Before Mangled
   char *gen_name; // Mangled Name
   Env *env;
   Type *type;
   Type *sizeof_type;
   struct Node *is_omiited;
   int is_static;
   int is_recursive;
   int pline;
   struct Node *funcdef;
   LocalVariable *local_variable;
} Node;

// Extended Register. with global variables, local variables, memory map,
// registers, ...
typedef struct Register {
   enum { R_REGISTER, R_LVAR, R_REVERSED_LVAR, R_GVAR, R_XMM } kind;
   int id; // offset or type
   int size;
   char *name; // for global variable
} Register;

Map *new_map();
int map_put(Map *map, const char *key, const void *val);
void *map_get(const Map *map, const char *key);
Vector *new_vector();
int vec_push(Vector *vec, Token *element);

void preprocess(Vector *pre_tokens, char *fname);

Vector *read_tokenize(char *fname);
Vector *tokenize(char *p);

Node *new_node(NodeType ty, Node *lhs, Node *rhs);
Node *new_num_node(long num_val);
int cnt_size(Type *type);

Node *analyzing(Node *node);
char *mangle_func_name(char *name);
Type *duplicate_type(Type *old_type);
Type *copy_type(Type *old_type, Type *type);
Type *find_typed_db(char *input, Map *db);
Type *find_typed_db_without_copy(char *input, Map *db);
Type *class_declaration(Map *local_typedb);
Type *new_type(void);
int type2size(Type *type);

void gen_register_top(void);
void init_reg_registers(void);
void globalvar_gen(void);

char *strdup(const char *s);

// util.c
void test_map(void);
_Noreturn void error(const char *str, ...);

#endif /* __HANANDO_FUKUI_MAIN__ */
