#include "../main.h"
#include <stdio.h>

void* malloc(int size);
int node2(Node* node, int j);

int getnode(Node* node, int j) {
   return node->args[j]->ty;
}

int main() {
   puts("aaa");
   int j=1;
   Node* node =  malloc(sizeof(Node));
   node->args[j]=malloc(sizeof(Node));
   node->args[j]->ty = 36;
   printf("%d\n", getnode(node, j));
   printf("%d\n", node2(node, j));
   return 0;
}
