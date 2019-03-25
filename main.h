#ifndef __HANANDO_FUKUI_MAIN__
#define __HANANDO_FUKUI_MAIN__

typedef enum {
   TK_ADD = '+',
   TK_SUB = '-',
   TK_NUM = 256,
   TK_IDENT,
   TK_EOF,
} TokenConst;

typedef enum {
   ND_ADD = '+',
   ND_SUB = '-',
   ND_MUL = '*',
   ND_DIV = '/',
   ND_LEFTPARENSIS = '(',
   ND_RIGHTPARENSIS = ')',
   ND_NUM = 256,
} NodeType;

typedef struct Node {
   NodeType ty;
   struct Node *lhs;
   struct Node *rhs;
   long num_val;
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

#endif /* __HANANDO_FUKUI_MAIN__ */
