int func2(char* input) {
   int k=0;
   for (k=0;input[k] != '\0';k++) {
      printf("%c ", input[k]);
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
   return 0;
}