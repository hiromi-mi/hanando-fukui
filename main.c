// main.c
/*


*/

#include "main.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

Vector *new_vector() {
   Vector* vec = malloc(sizeof(Vector));
   vec->capacity = 16;
   vec->data = malloc(sizeof(Token*) * vec->capacity);
   vec->len = 0;
   return vec;
}


void vec_push(Vector *vec, Token* element) {
   if (vec->capacity == vec->len) {
      vec->capacity *= 2;
      vec->data = realloc(vec->data, sizeof(Token*) * vec->capacity);
   }
   vec->data[vec->len++] = element;
}

// to use vector instead of something
Vector* tokens;
int pos = 0;
Node* code[100];

Node *new_node(NodeType ty, Node *lhs, Node *rhs) {
   Node *node = malloc(sizeof(Node));
   node->ty = ty;
   node->lhs = lhs;
   node->rhs = rhs;
   return node;
}

Node *new_num_node(long num_val) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_NUM;
   node->num_val = num_val;
   return node;
}

Node *new_ident_node(char name) {
   Node *node = malloc(sizeof(Node));
   node->ty = ND_IDENT;
   node->name = name;
   return node;
}

int consume_node(TokenConst ty) {
   if (tokens->data[pos]->ty != ty) {
      return 0;
   }
   pos++;
   return 1;
}

void tokenize(char *p) {
   tokens = new_vector();
   //int i = 0;
   while (*p != '\0') {
      if (isspace(*p)) {
         p++;
         continue;
      }
      if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' ||
            *p == ')' || *p == '=' || *p == ';') {
         Token *token = malloc(sizeof(Token));
         token->ty = *p;
         token->input = p;
         vec_push(tokens, token);
         // i++;
         p++;
         continue;
      }
      if (isdigit(*p)) {
         Token *token = malloc(sizeof(Token));
         token->ty = TK_NUM;
         token->input = p;
         token->num_val = strtol(p, &p, 10);
         vec_push(tokens, token);
         // i++;
         continue;
      }

      if ('a' <= *p && *p <= 'z') {
         Token *token = malloc(sizeof(Token));
         token->ty = TK_IDENT;
         token->input = p;
         vec_push(tokens, token);
         p++;
         // i++;
         continue;
      }

      fprintf(stderr, "Cannot Tokenize: %s\n", p);
      exit(1);
   }

   Token *token = malloc(sizeof(Token));
   token->ty = TK_EOF;
   token->input = p;
   vec_push(tokens, token);
}

Node *node_mul();
Node *node_term();
Node *node_add();

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

Node *node_mul() {
   Node *node = node_term();
   while (1) {
      if (consume_node('*')) {
         node = new_node('*', node, node_mul());
      } else if (consume_node('/')) {
         node = new_node('/', node, node_mul());
      } else {
         return node;
      }
   }
}

Node *node_term() {
   if (tokens->data[pos]->ty == TK_NUM) {
      Node *node = new_num_node(tokens->data[pos++]->num_val);
      return node;
   }
   if (tokens->data[pos]->ty == TK_IDENT) {
      Node *node = new_ident_node(*tokens->data[pos++]->input);
      return node;
   }
   if (consume_node('(')) {
      Node *node = node_add();
      if (!consume_node(')')) {
         puts("Error: Incorrect Parensis.");
         exit(1);
      }
      return node;
   }
   puts("Error: Incorrect Parensis.");
   exit(1);
}

void gen_lval(Node *node) {
   if (node->ty != ND_IDENT) {
      puts("eeror: Incorrect Variable of lvalue");
      exit(1);
   }

   int offset = ('z' - node->name + 1) * 8;
   puts("mov rax, rbp");
   printf("sub rax, %d\n", offset);
   puts("push rax");
}

void gen(Node *node) {
   if (node->ty == ND_NUM) {
      printf("push %ld\n", node->num_val);
      return;
   }

   if (node->ty == ND_IDENT) {
      gen_lval(node);
      puts("pop rax");
      puts("mov rax, [rax]");
      puts("push rax");
      return;
   }

   if (node->ty == '=') {
      gen_lval(node->lhs);
      gen(node->rhs);
      puts("pop rdi");
      puts("pop rax");
      puts("mov [rax], rdi");
      puts("push rdi");
      return;
   }

   gen(node->lhs);
   gen(node->rhs);

   puts("pop rdi");
   puts("pop rax");
   switch (node->ty) {
      case '+':
         puts("add rax, rdi");
         break;
      case '-':
         puts("sub rax, rdi");
         break;
      case '*':
         puts("mul rdi");
         break;
      case '/':
         puts("mov rdx, 0");
         puts("div rax, rdi");
         break;
      default:
         puts("Error");
         exit(1);
   }
   puts("push rax");
}


void expect(int line, int expected, int actual) {
   if (expected == actual)
      return;
   fprintf(stderr, "%d: %d expected, but got %d\n",
         line, expected, actual);
   exit(1);
}

Node *assign() {
   Node *node = node_add();
   if (consume_node('=')) {
      node = new_node('=', node, assign());
   }
   return node;
}

Node *stmt() {
   Node *node = assign();
   if (!consume_node(';')) {
      puts("Error: Not token ;");
      exit(1);
   }
   return node;
}

void program() {
   int i = 0;
   while(tokens->data[pos]->ty != TK_EOF) {
      code[i++] = stmt();
   }
   code[i] = NULL;
}

int main(int argc, char **argv) {
   if (argc < 2) {
      puts("Incorrect Arguments");
      exit(1);
   }

   //Vector *vec = new_vector();
   //vec_push(vec, 9);
   // assert(1 == vec->len);

   tokenize(argv[1]);
   program();
   //Node *node = node_add();
   // char *p = argv[1];

   puts(".intel_syntax");
   puts(".global main");
   puts("main:");

   puts("push rbp");
   puts("mov rbp, rsp");
   puts("sub rsp, 208");
   for (int i=0;code[i];i++) {
      gen(code[i]);
      puts("pop rax");
   }
   puts("mov rsp, rbp");
   puts("pop rbp");
   puts("ret");

   return 0;
}
