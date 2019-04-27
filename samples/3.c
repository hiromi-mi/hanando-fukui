#include "../main.h"
#include <stdio.h>

void* malloc(int size);
int node2(Node* node, int j);

int getnode(Node* node, int j) {
   puts(node->args[j]->name);
   return node->args[j]->ty;
}

int main() {
   puts("aaa");
   int j=1;
   Node* node =  malloc(sizeof(Node));
   node->args[j]=malloc(sizeof(Node));
   node->args[j]->ty = 36;
   node->args[j]->name = "bbb\n";
   printf("%d\n", getnode(node, j));
   printf("%d\n", node2(node, j));
   j=2;
   node->args[j]=malloc(sizeof(Node));
   node->args[j]->ty = 80;
   node->args[j]->name = malloc(sizeof(char) * 16);
   node->args[j]->name[0] = 'a';
   node->args[j]->name[1] = 's';
   node->args[j]->name[2] = 'y';
   node->args[j]->name[3] = 'm';
   node->args[j]->name[4] = '\0';
   node->args[j]->type = malloc(sizeof(Type));
   node->args[j]->type->ty = 30;
   printf("%d\n", getnode(node, j));
   printf("%d\n", node2(node, j));
   return 0;
}
