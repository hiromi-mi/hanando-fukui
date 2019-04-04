#ifndef __HANANDO_FUKUI_MAIN__
#define __HANANDO_FUKUI_MAIN__

typedef enum {
   TK_ADD = '+',
   TK_SUB = '-',
   TK_EQUAL = '=',
   TK_BLOCKBEGIN = '{',
   TK_BLOCKEND = '}',
   TK_NUM = 256,
   TK_IDENT,
   TK_EOF,
   TK_ISEQ,
   TK_ISNOTEQ,
   TK_IF,
   TK_WHILE,
   TK_ELSE,
   TK_RSHIFT,
   TK_LSHIFT,
   TK_OPAS,
   TK_PLUSPLUS = '+' + '+',
   TK_SUBSUB = '-' + '-',
   TK_TYPE,
   TK_RETURN,
} TokenConst;

typedef enum {
   ND_ADD = '+',
   ND_SUB = '-',
   ND_MUL = '*',
   ND_DIV = '/',
   ND_MOD = '%',
   ND_XOR = '^',
   ND_OR = '|',
   ND_AND = '&',
   ND_LESS = '<',
   ND_GREATER = '>',
   ND_LEFTPARENSIS = '(',
   ND_RIGHTPARENSIS = ')',
   ND_NUM = 256,
   ND_IDENT,
   ND_ISEQ,
   ND_ISNOTEQ,
   ND_FUNC,
   ND_FDEF,
   ND_RSHIFT,
   ND_LSHIFT,
   ND_INC,
   ND_DEC,
   ND_BLOCK,
   ND_IF,
   ND_WHILE,
   ND_RETURN,
} NodeType;

typedef struct Type {
   enum { TY_INT, TY_PTR } ty;
   struct Type *ptrof;
   int offset;
} Type;

typedef struct {
   TokenConst ty;
   long num_val;
   char *input;
} Token;

typedef struct {
   Token **data;
   int capacity;
   int len;
} Vector;

typedef struct {
   Vector *keys;
   Vector *vals;
} Map;

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
   struct Node *code[100];
   int argc;
   long num_val;
   char *name;
   Env *env;
} Node;

Map *new_map();
void map_put(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);
Vector *new_vector();
void vec_push(Vector *vec, Token *element);

#endif /* __HANANDO_FUKUI_MAIN__ */
