#include <stdio.h>

#ifdef __HANANDO_FUKUI__
FILE* fopen(char* name, char* type);
#endif
int main() {
   FILE* fp;
   fp = fopen("main.c", "r");
   fclose(fp);
   return 0;
}

