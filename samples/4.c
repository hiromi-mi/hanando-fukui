#include "../main.h"
int node2(Node* node, int j) {
   puts(node->args[j]->name);
   if (node->args[j]->type) {
      printf("%d\n", node->args[j]->type->ty);
   }
   return node->args[j]->ty;
}

