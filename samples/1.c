#include <stdio.h>

FILE* fopen(char* name, char* type);
int main() {
   FILE* fp;
   fp = fopen("main.c", "r");
   fclose(fp);
   return 0;
}

