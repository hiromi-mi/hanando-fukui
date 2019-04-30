#include "../main.h"
#include <stdio.h>

void* malloc(int size);
int node2(Node* node, int j);

int getnode(Node* node, int j) {
   {
      printf("%p\n", node->ty);
      printf("%p\n", node->lhs);
      printf("%p\n", node->rhs);
      printf("%p\n", node->args);
      printf("%p\n", node->code);
      printf("%p\n", node->argc);
      printf("%p\n", node->num_val);
      printf("%p\n", node->name);
      printf("%p\n", node->env);
      printf("%p\n", node->type);
   }
   puts(node->args[j]->name);
   return node->args[j]->ty;
}

int main() {
   puts("aaa");
   int j=1;
   Node* node =  malloc(sizeof(Node));
   node->args[j]=malloc(sizeof(Node));
   node->args[j]->ty = 36;
   node->args[j]->name = "afegerjger\n";
   printf("%d\n", getnode(node, j));
   printf("%d\n", node2(node, j));
   return 0;
}
