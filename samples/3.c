#include "../main.h"

void* malloc(int size);

int node2(Node* node, int j) {
   return node->args[j]->ty;
}

int main() {
   int j=1;
   Node* node =  malloc(sizeof(Node));
   node->args[j]=malloc(sizeof(Node));
   node->args[j]->ty = 30;
   return node2(node, j);
}
