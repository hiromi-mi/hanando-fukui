#ifndef __HANANDO_FUKUI_MAIN__
#define __HANANDO_FUKUI_MAIN__

typedef enum {
   TK_ADD = '+',
   TK_SUB = '-',
   TK_NUM = 256,
   TK_EOF,
} TokenConst;

typedef struct {
   TokenConst ty;
   long num_val;
   char *token_str;
} Token;

#endif /* __HANANDO_FUKUI_MAIN__ */
