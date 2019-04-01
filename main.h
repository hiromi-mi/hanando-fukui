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
   TK_ELSE,
   TK_RSHIFT,
   TK_LSHIFT,
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
} NodeType;

typedef struct Env {
   struct Env *env;
   struct Map *idents;
} Env;

typedef struct Node {
   NodeType ty;
   struct Node *lhs;
   struct Node *rhs;
   struct Node *args[100];
   int argc;
   long num_val;
   char* name;
   struct Env *env;
} Node;

typedef struct {
   TokenConst ty;
   long num_val;
   char *input;
} Token;

typedef struct {
   Token** data;
   int capacity;
   int len;
} Vector;

typedef struct {
   Vector* keys;
   Vector *vals;
} Map;

Map *new_map();
void map_put(Map *map, char* key, void* val);
void *map_get(Map *map, char* key);
Vector *new_vector();
void vec_push(Vector *vec, Token* element);

#endif /* __HANANDO_FUKUI_MAIN__ */
