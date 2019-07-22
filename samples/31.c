#include "../main.h"
int main() {
   int j=1;
   Node* node;
   node =  malloc(sizeof(Node));
   node->args[j]=malloc(sizeof(Node));
   node->args[j]->ty = 36;
   node->args[j]->name = "afegerjger\n";
   return 0;
}
