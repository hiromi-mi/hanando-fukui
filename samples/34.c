#include "../main.h"

Type *new_type() {
   Type *type = malloc(sizeof(Type));
   type->argc = 0;
   type->name = NULL;
   return type;
}

int main() {
   Type *concrete_type = new_type();
   concrete_type->args[concrete_type->argc++]->name = "aaa";
   return 0;
}
