extern int cd[30];

#ifdef __HANANDO_FUKUI__
FILE* fopen(char* name, char* type);
void* malloc(int size);
void* realloc(void* ptr, int size);
#endif
int func2(char* input) {
   int k=0;
   for (k=0;input[k] != '\0';k++) {
      printf("%c S ", input[k]);
      if (k > 10) {
   putchar('J');
         break;
      }
   }
   putchar('\n');
   return k;
}
int main() {
   int j = 0;
   char* p = "This is";
   func(p);
   func2(p);
   char* input;
   input = malloc(sizeof(char) * 256);
   printf("%c %d\n", *p, 'A' <= *p);
   do {
      input[j] = *p;
      p++;
      j++;
   } while (('a' <= *p && *p <= 'z') || ('0' <= *p && *p <= '9') ||
            ('A' <= *p && *p <= 'Z') || *p == '_');
   input[j] = '\0';
   int k=0;
   func(input);
   func2(input);
   printf("%d\n", cd[1]);
   if (cd[1] != 9) {
      puts("Error! extern does not work!");
      return 1;
   }
   cd[2]=4;
   printf("%d\n", cd[2]);
   return 0;
}
