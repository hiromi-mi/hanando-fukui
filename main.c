// main.c
/*


*/

#include "main.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double ceil(double x);
// to use vector instead of something
Vector *tokens;
int pos = 0;
Node *code[100];
int get_lval_offset(Node *node);
Type *get_type(Node *node);
void gen(Node *node);
Map *global_vars;
Map *funcdefs;
Vector *strs;

int type2size(Type *type);
Type *read_type(char **input);
int confirm_type();
int confirm_ident();
int split_type_ident();

Map *typedb;

void error(const char *str) {
   fprintf(stderr, "%s on %d: %s\n", str, pos, tokens->data[pos]->input);
   exit(1);
}

Node *new_string_node(char* id) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_STRING;
   node->lhs = NULL;
   node->rhs = NULL;
   node->name = id;
   Type *type = malloc(sizeof(Type));
   type->ty = TY_PTR;
   type->ptrof = malloc(sizeof(Type));
   type->ptrof->ty = TY_CHAR;
   type->ptrof->ptrof = NULL;
   node->type = type;
   return node;
}

Node *new_node(NodeType ty, Node *lhs, Node *rhs) {
   Node *node = malloc(sizeof(Node));
   node->ty = ty;
   node->lhs = lhs;
   node->rhs = rhs;
   if (lhs != NULL) {
      // TODO
      node->type = node->lhs->type;
   }
   return node;
}

Node *new_num_node(long num_val) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_NUM;
   node->num_val = num_val;
   node->type = malloc(sizeof(Type));
   node->type->ty = TY_INT;
   node->type->ptrof = NULL;
   node->type->array_size = -1;
   node->type->offset = -1;
   return node;
}

Env *env;
int if_cnt = 0;

Node *new_ident_node_with_new_variable(char *name, Type *type) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_IDENT;
   node->name = name;
   node->type = type;
   switch (type->ty) {
      case TY_PTR:
         env->rsp_offset += 8;
         break;
      case TY_INT:
         env->rsp_offset += 8;
         break;
      case TY_CHAR:
         env->rsp_offset += 8; // tekitou
      case TY_LONG:
         env->rsp_offset += 8; // tekitou
      case TY_ARRAY:
         // TODO: should not be 8 in case of truct
         env->rsp_offset += 8 * type->array_size;
         break;
      case TY_STRUCT:
         error("Struct is not supported");
         exit(1);
         break;
   }
   type->offset = env->rsp_offset;
   // type->ptrof = NULL;
   // type->ty = TY_INT;
   printf("#define: %s on %d\n", name, env->rsp_offset);
   map_put(env->idents, name, type);
   return node;
}

Node *new_ident_node(char *name) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_IDENT;
   node->name = name;
   node->type = get_type(node);
   if (node->type == NULL) {
      error("Error: New Variable Definition.");
   }
   return node;
}

Node *new_func_node(char *name) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_FUNC;
   node->name = name;
   node->lhs = NULL;
   node->rhs = NULL;
   node->type = NULL;
   node->argc = 0;
   // should support long?
   Node* result = map_get(funcdefs, name);
   if (result) {
      node->type = result->type;
   } else {
      node->type = malloc(sizeof(Type));
      node->type->ty = TY_INT;
   }
   return node;
}

Env *new_env(Env *prev_env) {
   Env *env = malloc(sizeof(Env));
   env->env = prev_env;
   env->idents = new_map();
   env->rsp_offset = 0;
   if (prev_env) {
      env->rsp_offset_all = prev_env->rsp_offset_all + prev_env->rsp_offset;
   } else {
      env->rsp_offset_all = 0;
   }
   return env;
}

Node *new_fdef_node(char *name, Env *prev_env, Type *type) {
   Node *node = malloc(sizeof(Node));
   node->type = type;
   node->ty = ND_FDEF;
   node->name = name;
   node->lhs = NULL;
   node->rhs = NULL;
   node->argc = 0;
   node->env = prev_env;
   map_put(funcdefs, name, node);
   return node;
}

Node *new_block_node(Env *prev_env) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_BLOCK;
   // node->name = name;
   node->lhs = NULL;
   node->rhs = NULL;
   node->argc = 0;
   node->type = NULL;
   node->env = new_env(prev_env);
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

Vector* tokenize(char *p) {
   Vector* pre_tokens = new_vector();
   while (*p != '\0') {
      if (*p == '\n') {
         Token *token = malloc(sizeof(Token));
         token->ty = TK_NEWLINE;
         vec_push(pre_tokens, token);
         while(*p == '\n' && *p != '\0') p++;
         continue;
      }
      if (isspace(*p)) {
         Token *token = malloc(sizeof(Token));
         token->ty = TK_SPACE;
         vec_push(pre_tokens, token);
         while(isspace(*p) && *p != '\0') p++;
         continue;
      }

      // TODO escape sequence.
      if (*p == '\"') {
         Token *token = malloc(sizeof(Token));
         // TODO
         token->input = malloc(sizeof(char) * 256);
         token->ty = TK_STRING;
         int i = 0;
         while (*++p != '\"') {
            token->input[i++] = *p;
         }
         token->input[i] = '\0';
         vec_push(pre_tokens, token);
         p++; // skip "
         continue;
      }
      // TODO escape sequence.
      if (*p == '\'') {
         Token *token = malloc(sizeof(Token));
         token->ty = TK_NUM;
         token->input = p;
         if (*(p+1) != '\\') {
            token->num_val = *(p + 1);
         } else {
            char str[16];
            snprintf(str, 16, "%s", p);
            token->num_val = str[0];
            p++;
         }
         vec_push(pre_tokens, token);
         p += 3; // *p = '', *p+1 = a, *p+2 = '
         continue;
      }
      if ((*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '%' ||
           *p == '^' || *p == '|' || *p == '&') &&
          (*(p + 1) == '=')) {
         {
            Token *token = malloc(sizeof(Token));
            token->ty = '=';
            token->input = p + 1;
            vec_push(pre_tokens, token);
         }
         {
            Token *token = malloc(sizeof(Token));
            token->ty = TK_OPAS;
            token->input = p;
            vec_push(pre_tokens, token);
         }
         p += 2;
         continue;
      }

      if ((*p == '+' && *(p + 1) == '+') || (*p == '-' && *(p + 1) == '-')) {
         Token *token = malloc(sizeof(Token));
         // TK_PLUSPLUS and TK_SUBSUB
         token->ty = *p + *(p + 1);
         token->input = p;
         vec_push(pre_tokens, token);
         p += 2;
         continue;
      }

      if ((*p == '|' && *(p + 1) == '|')) {
         Token *token = malloc(sizeof(Token));
         token->ty = TK_OR;
         token->input = p;
         vec_push(pre_tokens, token);
         p += 2;
         continue;
      }
      if ((*p == '&' && *(p + 1) == '&')) {
         Token *token = malloc(sizeof(Token));
         token->ty = TK_AND;
         token->input = p;
         vec_push(pre_tokens, token);
         p += 2;
         continue;
      }

      if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' ||
          *p == ')' || *p == ';' || *p == ',' || *p == '{' || *p == '}' ||
          *p == '%' || *p == '^' || *p == '|' || *p == '&' || *p == '?' ||
          *p == ':' || *p == '[' || *p == ']' || *p == '#') {
         Token *token = malloc(sizeof(Token));
         token->ty = *p;
         token->input = p;
         vec_push(pre_tokens, token);
         p++;
         continue;
      }
      if (*p == '=') {
         Token *token = malloc(sizeof(Token));
         token->input = p;
         if (*(p + 1) == '=') {
            token->ty = TK_ISEQ;
            p += 2;
         } else {
            token->ty = '=';
            p++;
         }
         vec_push(pre_tokens, token);
         continue;
      }
      if (*p == '!') {
         Token *token = malloc(sizeof(Token));
         token->input = p;
         if (*(p + 1) == '=') {
            token->ty = TK_ISNOTEQ;
            p += 2;
         } else {
            token->ty = *p;
            p++;
         }
         vec_push(pre_tokens, token);
         continue;
      }
      if (*p == '<') {
         Token *token = malloc(sizeof(Token));
         token->input = p;
         if (*(p + 1) == '<') {
            token->ty = TK_LSHIFT;
            p += 2;
         } else {
            token->ty = *p;
            p++;
         }
         vec_push(pre_tokens, token);
         continue;
      }
      if (*p == '>') {
         Token *token = malloc(sizeof(Token));
         token->input = p;
         if (*(p + 1) == '>') {
            token->ty = TK_RSHIFT;
            p += 2;
         } else {
            token->ty = *p;
            p++;
         }
         vec_push(pre_tokens, token);
         continue;
      }
      if (isdigit(*p)) {
         Token *token = malloc(sizeof(Token));
         token->ty = TK_NUM;
         token->input = p;
         token->num_val = strtol(p, &p, 10);
         vec_push(pre_tokens, token);
         continue;
      }

      if (('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z')) {
         Token *token = malloc(sizeof(Token));
         token->ty = TK_IDENT;
         token->input = malloc(sizeof(char) * 256);
         int j = 0;
         do {
            token->input[j] = *p;
            p++;
            j++;
         } while (('a' <= *p && *p <= 'z') || ('0' <= *p && *p <= '9') ||
                  ('A' <= *p && *p <= 'Z') || *p == '_');
         token->input[j] = '\0';

         if (strcmp(token->input, "if") == 0) {
            token->ty = TK_IF;
         }
         if (strcmp(token->input, "else") == 0) {
            token->ty = TK_ELSE;
         }
         if (strcmp(token->input, "while") == 0) {
            token->ty = TK_WHILE;
         }
         if (strcmp(token->input, "for") == 0) {
            token->ty = TK_FOR;
         }
         if (strcmp(token->input, "return") == 0) {
            token->ty = TK_RETURN;
         }
         if (strcmp(token->input, "sizeof") == 0) {
            token->ty = TK_SIZEOF;
         }
         if (strcmp(token->input, "goto") == 0) {
            token->ty = TK_GOTO;
         }
         if (strcmp(token->input, "struct") == 0) {
            token->ty = TK_STRUCT;
         }
         if (strcmp(token->input, "typedef") == 0) {
            token->ty = TK_TYPEDEF;
         }

         vec_push(pre_tokens, token);
         continue;
      }

      fprintf(stderr, "Cannot Tokenize: %s\n", p);
      // DEBUG
      /*
      for (int j=0;j<=pre_tokens->len-1;j++) {
         fprintf(stderr, "%c %d %s\n", pre_tokens->data[j]->ty, pre_tokens->data[j]->ty,
      pre_tokens->data[j]->input);
      }
      */
   }

   Token *token = malloc(sizeof(Token));
   token->ty = TK_EOF;
   token->input = p;
   vec_push(pre_tokens, token);
   return pre_tokens;
}

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
Node *assign();

Node *node_mathexpr_without_comma() {
   return node_lor();
}

Node *node_mathexpr() {
   Node* node = node_lor();
   while (1) {
      if (consume_node(',')) {
         node = new_node(',', node, node_lor());
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
         node = new_node('<', node, node_shift());
      } else if (consume_node('>')) {
         node = new_node('>', node, node_shift());
      } else {
         return node;
      }
   }
}

Node *node_and() {
   Node *node = node_compare();
   while (1) {
      if (consume_node('&')) {
         node = new_node('&', node, node_compare());
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
         node = new_node('+', node, node_mul());
      } else if (consume_node('-')) {
         node = new_node('-', node, node_mul());
      } else {
         return node;
      }
   }
}

Node *node_cast() {
   if (consume_node(TK_PLUSPLUS)) {
      Node *node = new_ident_node(tokens->data[pos]->input);
      expect_node(TK_IDENT);
      return new_node(ND_INC, node, NULL);
   } else if (consume_node(TK_SUBSUB)) {
      Node *node = new_ident_node(tokens->data[pos]->input);
      expect_node(TK_IDENT);
      return new_node(ND_DEC, node, NULL);
   } else if (consume_node('&')) {
      return new_node(ND_ADDRESS, node_mathexpr(), NULL);
   } else if (consume_node('*')) {
      return new_node(ND_DEREF, node_mathexpr(), NULL);
   } else if (consume_node(TK_SIZEOF)) {
      if (consume_node('(') && confirm_type()) {
         // sizeof(int) read type without name
         Type *type = read_type(NULL);
         expect_node(')');
         return new_num_node(type2size(type));
      }
      Node *node = node_mathexpr();
      return new_num_node(type2size(node->type));
   } else {
      return node_term();
   }
}

Node *node_mul() {
   Node *node = node_cast();
   while (1) {
      if (consume_node('*')) {
         node = new_node('*', node, node_cast());
      } else if (consume_node('/')) {
         node = new_node('/', node, node_cast());
      } else if (consume_node('%')) {
         node = new_node('%', node, node_cast());
      } else {
         return node;
      }
   }
}

Node *node_term() {
   if (confirm_node(TK_NUM)) {
      Node *node = new_num_node(tokens->data[pos]->num_val);
      expect_node(TK_NUM);
      return node;
   }
   if (confirm_ident()) {
      Node *node;
      // Function Call
      if (tokens->data[pos + 1]->ty == '(') {
         node = new_func_node(tokens->data[pos]->input);
         // skip func , (
         expect_node(TK_IDENT);
         expect_node('(');
         while (1) {
            if (!consume_node(',') && consume_node(')')) {
               break;
            }
            node->args[node->argc++] = node_mathexpr_without_comma();
         }
         assert(node->argc <= 6);
         // pos++ because of consume_node(')')
         // array
      } else if (tokens->data[pos + 1]->ty == '[') {
         char *input = tokens->data[pos]->input;
         expect_node(TK_IDENT);
         expect_node('[');
         node = new_node(ND_DEREF,
                         new_node('+', new_ident_node(input), node_mathexpr()),
                         NULL);
         expect_node(']');
      } else {
         // Just an ident
         node = new_ident_node(tokens->data[pos]->input);
         expect_node(TK_IDENT);
         if (consume_node(TK_PLUSPLUS)) {
            node = new_node(ND_FPLUSPLUS, node, NULL);
         } else if (consume_node(TK_SUBSUB)) {
            node = new_node(ND_FSUBSUB, node, NULL);
         }
      }
      return node;
   }
   if (confirm_node(TK_STRING)) {
      char *str = malloc(sizeof(char) *256);
      snprintf(str, 255, ".LC%d", vec_push(strs, tokens->data[pos]->input)); 
      Node* node = new_string_node(str);
      expect_node(TK_STRING);
      return node;
   }
   // Parensis
   if (consume_node('(')) {
      Node *node = assign();
      if (!consume_node(')')) {
         error("Error: Incorrect Parensis.");
      }
      return node;
   }
   fprintf(stderr, "Error: Incorrect Paresis without %c %d -> %c %d\n",
           tokens->data[pos - 1]->ty, tokens->data[pos - 1]->ty,
           tokens->data[pos]->ty, tokens->data[pos]->ty);
   exit(1);
}

Type *get_type_local(Node *node) {
   Env *local_env = env;
   Type *type = NULL;
   while (type == NULL && local_env != NULL) {
      type = map_get(local_env->idents, node->name);
      if (type != NULL) {
         break;
      }
      local_env = local_env->env;
   }
   return type;
}

Type *get_type(Node *node) {
   Type *type = get_type_local(node);
   if (type == NULL) {
      type = map_get(global_vars, node->name);
   }
   return type;
}

// TODO: another effect: set node->type. And join into get_type()
int get_lval_offset(Node *node) {
   int offset = (int)NULL;
   Env *local_env = env;
   while (offset == (int)NULL && local_env != NULL) {
      node->type = map_get(local_env->idents, node->name);
      if (node->type != NULL) {
         offset = local_env->rsp_offset_all + node->type->offset;
      }
      local_env = local_env->env;
   }
   return offset;
}

char* rax(Node *node) {
   if (node->type->ty == TY_CHAR) {
      return "al";
   } else if (node->type->ty == TY_INT) {
      return "eax";
   } else {
      return "rax";
   }
}

char* rdi(Node *node) {
   if (node->type->ty == TY_CHAR) {
      return "dil";
   } else if (node->type->ty == TY_INT) {
      return "edi";
   } else {
      return "rdi";
   }
}

void gen_lval(Node *node) {
   if (node->ty == ND_IDENT) {
      int offset = get_lval_offset(node);
      if (offset != (int)NULL) {
         puts("mov rax, rbp");
         printf("sub rax, %d\n", offset);
      } else {
         // treat as global variable.
         node->type = get_type(node);
         printf("lea rax, dword ptr %s[rip]\n", node->name);
      }
      puts("push rax");
      return;
   }
   if (node->ty == ND_DEREF) {
      gen(node->lhs);
      return;
   }

   error("Error: Incorrect Variable of lvalue");
}

int type2size(Type *type) {
   if (type == NULL) {
      return 0;
   }
   switch (type->ty) {
      case TY_PTR:
         return 8;
      case TY_LONG:
         return 8;
      case TY_INT:
         return 4;
      case TY_CHAR:
         return 1;
      case TY_ARRAY:
         return type->array_size * type2size(type->ptrof);
      case TY_STRUCT: {
         int val = 0;
         for (int j = 0; j < type->structure->keys->len; j++) {
            val += type2size(type->structure->keys->data[j]);
         }
         return val;
      }
   }
}

char *type2string(Node *node) {
   Type *type = node->type;
   if (type == NULL) {
      return 0;
   }
   switch (type->ty) {
      case TY_PTR:
         return "";
      case TY_LONG:
         return "";
      case TY_INT:
         return "";
      case TY_CHAR:
         return "byte ptr ";
      case TY_ARRAY:
         return "";
      default:
         error("Error: NOT a type");
         return "";
   }
}

void gen(Node *node) {
   if (node->ty == ND_NUM) {
      printf("push %ld\n", node->num_val);
      return;
   }
   if (node->ty == ND_STRING) {
      printf("lea rax, dword ptr %s[rip]\n", node->name);
      puts("push rax");
      return;
   }

   if (node->ty == ND_BLOCK) {
      Env *prev_env = env;
      env = node->env;
      // printf("%s:\n", node->name);
      for (int j = 0; node->code[j] != NULL; j++) {
         // read inside functions.
         if (node->code[j]->ty == ND_RETURN) {
            gen(node->code[j]->lhs);
            puts("pop rax");
            puts("mov rsp, rbp");
            puts("pop rbp");
            puts("ret");
            break;
         }
         gen(node->code[j]);
      }
      env = prev_env;
      return;
   }

   if (node->ty == ND_FDEF) {
      Env *prev_env = env;
      env = node->env;
      printf("%s:\n", node->name);
      puts("push rbp");
      puts("mov rbp, rsp");
      printf("sub rsp, %d\n", env->rsp_offset);
      char registers[6][4] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
      for (int j = 0; j < node->argc; j++) {
         gen_lval(node->args[j]);
         puts("pop rax");
         printf("mov [rax], %s\n", registers[j]);
         puts("push rax");
      }
      for (int j = 0; node->code[j] != NULL; j++) {
         if (node->code[j]->ty == ND_RETURN) {
            gen(node->code[j]->lhs);
            puts("pop rax");
            break;
         }
         // read inside functions.
         gen(node->code[j]);
         puts("pop rax");
      }
      puts("mov rsp, rbp");
      puts("pop rbp");
      puts("ret");
      env = prev_env;
      return;
   }

   if (node->ty == ND_LOR) {
      int cur_if_cnt = if_cnt++;
      gen(node->lhs);
      puts("pop rdi");
      puts("cmp rdi, 0");
      printf("jne .Lorend%d\n", cur_if_cnt);
      gen(node->rhs);
      puts("pop rdi");
      puts("cmp rdi, 0");
      printf(".Lorend%d:\n", cur_if_cnt);
      puts("setne al");
      puts("movzx rax, al");
      puts("push rax");
      return;
   }
   if (node->ty == ND_LAND) {
      int cur_if_cnt = if_cnt++;
      gen(node->lhs);
      puts("pop rdi");
      puts("cmp rdi, 0");
      printf("je .Lorend%d\n", cur_if_cnt);
      gen(node->rhs);
      puts("pop rdi");
      puts("cmp rdi, 0");
      printf(".Lorend%d:\n", cur_if_cnt);
      puts("setne al");
      puts("movzx rax, al");
      puts("push rax");
      return;
   }

   if (node->ty == ND_FUNC) {
      char registers[6][4] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
      for (int j = 0; j < node->argc; j++) {
         gen(node->args[j]);
         printf("pop %s\n", registers[j]);
      }
      printf("sub rsp, %d\n", (int)(ceil(4 * node->argc / 16.)) * 16);
      // FIXME: alignment should be 64-bit
      puts("mov al, 0"); // TODO to preserve float
      printf("call %s\n", node->name);
      puts("push rax");
      return;
   }

   if (node->ty == ND_IF) {
      gen(node->args[0]);
      puts("pop rax");
      puts("cmp rax, 0");
      printf("je .Lend%d\n", if_cnt);
      gen(node->lhs);
      if (node->rhs) {
         printf("jmp .Lelseend%d\n", if_cnt);
      }
      printf(".Lend%d:\n", if_cnt);
      if (node->rhs) {
         // There are else
         gen(node->rhs);
         printf(".Lelseend%d:\n", if_cnt);
      }
      if_cnt++;
      return;
   }

   if (node->ty == ND_WHILE) {
      int cur_if_cnt = if_cnt++;
      printf(".Lbegin%d:\n", cur_if_cnt);
      gen(node->lhs);
      puts("pop rax");
      puts("cmp rax, 0");
      printf("je .Lend%d\n", cur_if_cnt);
      gen(node->rhs);
      printf("jmp .Lbegin%d\n", cur_if_cnt);
      printf(".Lend%d:\n", cur_if_cnt);
      return;
   }

   if (node->ty == ND_FOR) {
      int cur_if_cnt = if_cnt++;
      gen(node->args[0]);
      printf("jmp .Lcondition%d\n", cur_if_cnt);
      printf(".Lbegin%d:\n", cur_if_cnt);
      gen(node->rhs);
      gen(node->args[2]);
      printf(".Lcondition%d:\n", cur_if_cnt);
      gen(node->args[1]);

      // condition
      puts("pop rax");
      puts("cmp rax, 0");
      printf("jne .Lbegin%d\n", cur_if_cnt);
      return;
   }

   if (node->ty == ND_IDENT) {
      gen_lval(node);
      if (node->type->ty != TY_ARRAY) {
         // deref
         puts("pop rax");
         puts("mov rax, [rax]");
         puts("push rax");
      }
      return;
   }

   if (node->ty == ND_DEREF) {
      gen(node->lhs); // Compile as RVALUE
      puts("#deref");
      puts("pop rax");
      puts("mov rax, [rax]");
      puts("push rax");
      puts("\n");
      return;
   }

   if (node->ty == ND_ADDRESS) {
      gen_lval(node->lhs);
      node->type = malloc(sizeof(Type));
      node->type->ty = TY_PTR;
      return;
   }

   if (node->ty == ND_GOTO) {
      printf("jmp %s\n", node->name);
      return;
   }

   if (node->ty == '=') {
      gen_lval(node->lhs);
      gen(node->rhs);
      puts("pop rdi");
      puts("pop rax");
      if (node->type->ty == TY_CHAR) {
         puts("mov [rax], dil");
      } else {
         puts("mov [rax], rdi");
      }
      puts("push rdi");
      return;
   }

   if (node->ty == ND_FPLUSPLUS) {
      gen_lval(node->lhs);
      puts("pop rax");
      puts("mov rdi, [rax]");
      puts("push rdi");
      puts("add rdi, 1");
      puts("mov [rax], rdi");
      return;
   }
   if (node->ty == ND_FSUBSUB) {
      gen_lval(node->lhs);
      puts("pop rax");
      puts("mov rdi, [rax]");
      puts("push rdi");
      puts("sub rdi, 1");
      puts("mov [rax], rdi");
      return;
   }

   if (node->ty == ND_INC) {
      gen_lval(node->lhs);
      puts("pop rax");
      puts("mov rdi, [rax]");
      puts("add rdi, 1");
      puts("mov [rax], rdi");
      puts("push rdi");
      return;
   }
   if (node->ty == ND_DEC) {
      gen_lval(node->lhs);
      puts("pop rax");
      puts("mov rdi, [rax]");
      puts("sub rdi, 1");
      puts("mov [rax], rdi");
      puts("push rdi");
      return;
   }

   gen(node->lhs);
   gen(node->rhs);

   if (node->lhs->type &&
       (node->lhs->type->ty == TY_ARRAY || node->lhs->type->ty == TY_PTR)) {
      puts("pop rax"); // rhs
      puts("pop rdi"); // lhs because of mul
      printf("mov r10, %d\n", type2size(node->lhs->type));
      puts("mul r10");
      switch (node->ty) {
         case '+':
            puts("add rax, rdi");
            break;
         case '-':
            puts("sub rax, rdi"); // stack
            break;
         default:
            error("Error: Not supported pointer eq.");
      }
      puts("push rax");
      return;
   }
   if (node->rhs->type &&
       (node->rhs->type->ty == TY_ARRAY || node->rhs->type->ty == TY_PTR)) {
      puts("pop rdi"); // rhs
      puts("pop rax"); // lhs
      printf("mov r10, %d\n", type2size(node->rhs->type));
      puts("mul r10"); // TODO multiply pointer size (8)
      switch (node->ty) {
         case '+':
            puts("add rax, rdi");
            break;
         case '-':
            puts("sub rax, rdi"); // stack
            break;
         default:
            error("Error: Not supported pointer eq.");
      }
      puts("push rax");
      return;
   }

   puts("pop rdi"); // rhs
   puts("pop rax"); // lhs
   switch (node->ty) {
      case ',':
         puts("mov rax, rdi");
         break;
      case '+':
         printf("add %s, %s\n", rax(node), rdi(node));
         //puts("add rax, rdi");
         break;
      case '-':
         printf("sub %s, %s\n", rax(node), rdi(node));
         //puts("sub rax, rdi");
         break;
      case '*':
         puts("mul rdi");
         break;
      case '/':
         puts("mov rdx, 0");
         puts("div rax, rdi");
         break;
      case '%':
         puts("mov rdx, 0");
         puts("div rax, rdi");
         // modulo are stored in rdx.
         puts("mov rax, rdx");
         break;
      case '^':
         puts("xor rax, rdi");
         break;
      case '&':
         puts("and rax, rdi");
         break;
      case '|':
         puts("or rax, rdi");
         break;
      case ND_RSHIFT:
         // FIXME: for signed int (Arthmetric)
         // mov rdi[8] -> rax
         puts("mov cl, dil");
         puts("sar rax, cl");
         break;
      case ND_LSHIFT:
         puts("mov cl, dil");
         puts("sal rax, cl");
         break;
      case ND_ISEQ:
         puts("cmp rdi, rax");
         puts("sete al");
         puts("movzx rax, al");
         break;
      case ND_ISNOTEQ:
         puts("cmp rdi, rax");
         puts("setne al");
         puts("movzx rax, al");
         break;
      case '>':
         puts("cmp rdi, rax");
         puts("setb al");
         puts("movzx rax, al");
         break;
      case '<':
         // reverse of < and >
         puts("cmp rdi, rax");
         // TODO: is "andb 1 %al" required?
         puts("setg al");
         puts("movzx rax, al");
         break;
      default:
         error("Error: no generations found.");
   }
   puts("push rax");
}

void expect(int line, int expected, int actual) {
   if (expected == actual)
      return;
   fprintf(stderr, "%d: %d expected, but got %d\n", line, expected, actual);
   exit(1);
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
         node = new_node('=', node, new_node(tp, node, assign()));
      } else {
         node = new_node('=', node, assign());
      }
   } else if (consume_node(TK_ISEQ)) {
      node = new_node(ND_ISEQ, node, assign());
   } else if (consume_node(TK_ISNOTEQ)) {
      node = new_node(ND_ISNOTEQ, node, assign());
   }
   return node;
}

Type *read_type(char **input) {
   if (!confirm_type()) {
      error("Error: NOT a type when reading type.");
      return NULL;
   }

   Type *type = malloc(sizeof(Type));
   // Variable Definition.
   int typekind = 10;
   split_type_ident();
   if (strcmp(tokens->data[pos]->input, "int") == 0) {
      typekind = TY_INT;
   } else if (strcmp(tokens->data[pos]->input, "char") == 0) {
      typekind = TY_CHAR;
   } else if (strcmp(tokens->data[pos]->input, "long") == 0) {
      typekind = TY_LONG;
   }
   type->ty = typekind;
   type->ptrof = NULL;
   // if there are type def.
   // skip this name.
   pos++;
   Type *rectype = type;
   // consume is pointer or not
   while (consume_node('*')) {
      puts("# new use pointer\n");
      Type *old_rectype = rectype;
      rectype = malloc(sizeof(Type));
      rectype->ty = typekind;
      rectype->ptrof = NULL;
      old_rectype->ty = TY_PTR;
      old_rectype->ptrof = rectype;
   }
   // There are input: there are ident names
   if (input != NULL) {
      *input = tokens->data[pos]->input;
   } else {
      input = &tokens->data[pos]->input;
   }
   // skip the name  of position.
   consume_node(TK_IDENT);
   // pos++;
   // array
   if (consume_node('[')) {
      type->ty = TY_ARRAY;
      // TODO: support NOT functioned type
      // ex. int a[4+7];
      type->array_size = (int)tokens->data[pos]->num_val;
      Type *rectype;
      rectype = malloc(sizeof(Type));
      rectype->ty = typekind;
      rectype->ptrof = NULL;
      type->ptrof = rectype;
      expect_node(TK_NUM);
      expect_node(']');
   }
   return type;
}

Node *stmt() {
   Node *node = NULL;
   if (confirm_type()) {
      char *input = NULL;
      Type *type = read_type(&input);
      node = new_ident_node_with_new_variable(input, type);
      // if there is int a =1;
      if (consume_node('=')) {
         node = new_node('=', node, node_mathexpr());
      }
   } else if (consume_node(TK_RETURN)) {
      node = new_node(ND_RETURN, assign(), NULL);
      // FIXME GOTO is not statement, expr.
   } else if (consume_node(TK_GOTO)) {
      node = new_node(ND_GOTO, NULL, NULL);
      // TODO should fix
      node->name = tokens->data[pos]->input;
      expect_node(TK_IDENT);
   } else {
      node = assign();
   }
   if (!consume_node(';')) {
      error("Error: Not token ;");
   }
   return node;
}
int i = 0;

void program(Node *block_node) {
   expect_node('{');
   Node **args = block_node->code;
   Env *prev_env = env;
   env = block_node->env;
   // env = new_env(env);
   while (!consume_node('}')) {
      if (confirm_node('{')) {
         args[0] = new_block_node(env);
         program(args[0]);
         args++;
         continue;
      }

      if (consume_node(TK_IF)) {
         args[0] = new_node(ND_IF, NULL, NULL);
         args[0]->argc = 1;
         args[0]->args[0] = assign();
         args[0]->args[1] = NULL;
         // Suppress COndition

         args[0]->lhs = new_block_node(env);
         program(args[0]->lhs);
         if (consume_node(TK_ELSE)) {
            args[0]->rhs = new_block_node(env);
            program(args[0]->rhs);
         } else {
            args[0]->rhs = NULL;
         }
         args++;
         continue;
      }
      if (consume_node(TK_WHILE)) {
         args[0] = new_node(ND_WHILE, node_mathexpr(), NULL);
         args[0]->rhs = new_block_node(env);
         program(args[0]->rhs);
         args++;
         continue;
      }
      if (consume_node(TK_FOR)) {
         args[0] = new_node(ND_FOR, NULL, NULL);
         expect_node('(');
         args[0]->args[0] = assign();
         expect_node(';');
         args[0]->args[1] = assign();
         expect_node(';');
         args[0]->args[2] = assign();
         expect_node(')');
         args[0]->rhs = new_block_node(env);
         program(args[0]->rhs);
         args++;
         continue;
      }
      args[0] = stmt();
      args++;
      // args[i++] = stmt();
   }
   args[0] = NULL;

   // 'consumed }'
   env = prev_env;
}

// 0: neither 1:TK_TYPE 2:TK_IDENT
int split_type_ident() {
   Token *token = tokens->data[pos];
   if (token->ty != TK_IDENT) {
      return 0;
   }
   // ident or type
   if (strcmp(token->input, "int") == 0) {
      return TY_INT;
   }
   if (strcmp(token->input, "char") == 0) {
      return TY_CHAR;
   }
   if (strcmp(token->input, "long") == 0) {
      return TY_LONG;
   }
   for (int j = 0; j < typedb->keys->len; j++) {
      // for struct
      if (strcmp(token->input, typedb->keys->data[j]) == 0) {
         return TY_STRUCT;
      }
   }
   return 2;
}

int confirm_type() {
   if (split_type_ident() > 2) {
      return 1;
   }
   return 0;
}
int confirm_ident() {
   if (split_type_ident() == 2) {
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

void toplevel() {
   // consume_node('{')
   // idents = new_map...
   // stmt....
   // consume_node('}')
   global_vars = new_map();
   funcdefs = new_map();
   strs = new_vector();
   env = new_env(NULL);
   while (tokens->data[pos]->ty != TK_EOF) {
      // definition of struct
      if (consume_node(TK_TYPEDEF) && consume_node(TK_STRUCT)) {
         consume_node(TK_IDENT); // for ease
         expect_node('{');
         Type *structuretype = malloc(sizeof(Type));
         structuretype->structure = new_map();
         structuretype->ty = TY_STRUCT;
         structuretype->ptrof = NULL;
         while (!consume_node('}')) {
            char *name = NULL;
            Type *type = read_type(&name);
            expect_node(';');
            map_put(structuretype->structure, name, type);
         }
         char *name = expect_ident();
         expect_node(';');
         fprintf(stderr, "#define new struct: %s\n", name);
         map_put(typedb, name, structuretype);
         continue;
      }
      if (confirm_type()) {
         char *name = NULL;
         Type *type = read_type(&name);
         if (consume_node('(')) {
            code[i] = new_fdef_node(name, env, type);
            // Function definition
            // because toplevel func call
            for (code[i]->argc = 0; code[i]->argc <= 6 && !consume_node(')');) {
               char *arg_name = NULL;
               Type *arg_type = read_type(&arg_name);
               // expect_node(TK_TYPE);
               /*
               Type *type = malloc(sizeof(Type));
               type->ty = TY_INT;
               type->ptrof = NULL;
               */
               code[i]->args[code[i]->argc++] =
                   new_ident_node_with_new_variable(arg_name, arg_type);
               consume_node(',');
            }
            // to support prototype def.
            if (confirm_node('{')) {
               program(code[i++]);
            } else {
               expect_node(';');
            }
            continue;
         } else {
            // global variable
            map_put(global_vars, name, type);
            // get initial value
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
         code[i] = new_block_node(NULL);
         program(code[i++]);
         continue;
      }
      code[i++] = stmt();
   }
   code[i] = NULL;
}

void test_map() {
   Map *map = new_map();
   expect(__LINE__, 0, (int)map_get(map, "foo"));
   map_put(map, "foo", (void *)2);
   expect(__LINE__, 2, (int)map_get(map, "foo"));
}

void globalvar_gen() {
   puts(".data");
   for (int j = 0; j < global_vars->keys->len; j++) {
      if (((Type *)global_vars->vals->data[j])->initval != 0) {
         printf(".type %s,@object\n", (char *)global_vars->keys->data[j]);
         printf(".size %s, 4\n", (char *)global_vars->keys->data[j]);
         printf("%s:\n", (char *)global_vars->keys->data[j]);
         printf(".long %d\n", ((Type *)global_vars->vals->data[j])->initval);
         puts(".text");
         // global_vars->vals->data[j];
      } else {
         printf(".comm %s, 4\n", (char *)global_vars->keys->data[j]);
      }
   }
   for (int j = 0; j < strs->len; j++) {
      printf(".LC%d:\n", j);
      printf(".string \"%s\"\n", (char *)strs->data[j]);
   }
}

void preprocess(Vector* pre_tokens) {
   tokens = new_vector();
   Map* defined = new_map();
   for (int j=0;j<=pre_tokens->len-1;j++) {
      if (pre_tokens->data[j]->ty == '#') {
         // preprocessor begin
         j++;
         if (strcmp(pre_tokens->data[j]->input, "define") == 0) {
            map_put(defined, pre_tokens->data[j+2]->input, pre_tokens->data[j+4]);
            while(pre_tokens->data[j]->ty != TK_NEWLINE && pre_tokens->data[j]->ty != TK_EOF) {
               j++;
            }
            /*
            Token* last_token = pre_tokens->data[j-1];
            last_token
            */
            //j+=4;
         }
         if (strcmp(pre_tokens->data[j]->input, "include") == 0) {
            //pre_tokens->data[j]->ty == TK_STRING
            j++;
         }
         continue;
      }
      if( pre_tokens->data[j]->ty == TK_NEWLINE) continue;
      if( pre_tokens->data[j]->ty == TK_SPACE) continue;
      int called = 0;
      for (int k=0;k<=defined->keys->len-1;k++) {
         if ( strcmp(pre_tokens->data[j]->input, defined->keys->data[k]) == 0) {
            called = 1;
            fprintf(stderr, "#define changed: %s -> %s\n", pre_tokens->data[j]->input, defined->vals->data[k]->input);
            //pre_tokens->data[j] = defined->vals->data[k];
            vec_push(tokens, defined->vals->data[k]);
            continue;
         }
      }
      if (!called) {
         vec_push(tokens, pre_tokens->data[j]);
      }
   }
   Token *token = malloc(sizeof(Token));
   token->ty = TK_EOF;
   token->input = "";
   vec_push(pre_tokens, token);
   vec_push(tokens, token);
}

int main(int argc, char **argv) {
   if (argc < 2) {
      error("Incorrect Arguments");
      exit(1);
   }

   test_map();

   // Vector *vec = new_vector();
   // vec_push(vec, 9);
   // assert(1 == vec->len);
   typedb = new_map();

   preprocess(tokenize(argv[1]));
   toplevel();

   puts(".intel_syntax");

   puts(".align 4");
   puts(".global main");
   puts(".type main, @function");
   // treat with global variables
   globalvar_gen();

   for (int j = 0; code[j]; j++) {
      gen(code[j]);
   }

   return 0;
}
