#include "../main.h"

void* malloc(int size);
int main() {
   Node* node =  malloc(sizeof(Node));
   node->args[1]=malloc(sizeof(Node));
   node->args[1]->ty = TY_VOID;
   return 0;
}
