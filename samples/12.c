int func(char* input) {
   int k=0;
   for (k=0;input[k] != '\0';k++) {
      printf("%c ", input[k]);
   }
   putchar('\n');
   return k;
}
