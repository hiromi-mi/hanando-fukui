// main.c
/*


*/

#include "main.h"
#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// to use vector instead of something
Token tokens[100];
int pos = 0;

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

int consume_node(TokenConst ty) {
   if (tokens[pos].ty != ty) {
      return 0;
   }
   pos++;
   return 1;
}

void tokenize(char *p) {
   int i = 0;
   while (*p != '\0') {
      if (isspace(*p)) {
         p++;
         continue;
      }
      if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' ||
          *p == ')') {
         tokens[i].ty = *p;
         tokens[i].token_str = p;
         i++;
         p++;
         continue;
      }
      if (isdigit(*p)) {
         tokens[i].ty = TK_NUM;
         tokens[i].token_str = p;
         tokens[i].num_val = strtol(p, &p, 10);
         i++;
         continue;
      }

      fprintf(stderr, "Cannot Tokenize: %s\n", p);
      exit(1);
   }

   tokens[i].ty = TK_EOF;
   tokens[i].token_str = p;
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
   if (tokens[pos].ty == TK_NUM) {
      Node *node = new_num_node(tokens[pos++].num_val);
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

void gen(Node *node) {
   if (node->ty == ND_NUM) {
      printf("push %ld\n", node->num_val);
      return;
   } else {
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
}

int main(int argc, char **argv) {
   if (argc < 2) {
      puts("Incorrect Arguments");
      exit(1);
   }

   tokenize(argv[1]);
   Node *node = node_add();
   // char *p = argv[1];

   puts(".intel_syntax");
   puts(".global main");
   puts("main:");
   gen(node);
   puts("pop rax\nret");
   return 0;
}
