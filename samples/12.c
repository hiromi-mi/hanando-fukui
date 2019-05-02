int cd[30];

int func(char* input) {
   cd[1]=9;
   int k=0;
   for (k=0;input[k] != '\0';k++) {
      printf("%c ", input[k]);
   }
   putchar('\n');
   return k;
}
