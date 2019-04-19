#ifndef __HANANDO_FUKUI_MAIN__
#define __HANANDO_FUKUI_MAIN__

typedef enum {
   TK_ADD = '+',
   TK_SUB = '-',
   TK_EQUAL = '=',
   TK_BLOCKBEGIN = '{',
   TK_BLOCKEND = '}',
   TK_S = '#',
   TK_PLUSPLUS = '+' + '+',
   TK_SUBSUB = '-' + '-',
   TK_NUM = 256,
   TK_IDENT,
   TK_EOF,
   TK_ISEQ,
   TK_ISNOTEQ,
   TK_IF,
   TK_WHILE,
   TK_FOR,
   TK_ELSE,
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
   ND_COMMA = ',',
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
} NodeType;

typedef struct {
   TokenConst ty;
   long num_val;
   char *input;
   int pos;
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

typedef struct Type {
   enum { TY_INT = 3, TY_PTR, TY_ARRAY, TY_CHAR, TY_LONG, TY_STRUCT } ty;
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
   struct Node *code[100];
   int argc;
   long num_val;
   char *name;
   Env *env;
   Type *type;
} Node;

Map *new_map();
int map_put(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);
Vector *new_vector();
int vec_push(Vector *vec, Token *element);

#endif /* __HANANDO_FUKUI_MAIN__ */
