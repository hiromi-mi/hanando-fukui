#include <stdio.h>
#include "../main.h"
int node2(Node* node, int j) {
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
   printf("%s\n", node->args[j]->name);
   return node->args[j]->ty;
}

