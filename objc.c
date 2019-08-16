typedef struct ID {
   struct ID *isa;
   struct void (**vtable);
} ID;

typedef struct SEL {
   char* name;
} SEL;

void* objc_msgsend(ID self, SEL cmd) {
   ID 
   cmd->name;
}
