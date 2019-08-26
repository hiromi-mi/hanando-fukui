#include <string.h>
#include <stdlib.h>
int main() {
   char* x[3] = {"Tokyo", "Yokohama", "Osaka"};

   if (strcmp(x[0], "Tokyo") != 0) {
      exit(1);
   }
   if (strcmp(x[1], "Yokohama") != 0) {
      exit(1);
   }
   if (strcmp(x[2], "Osaka") != 0) {
      exit(1);
   }
   return 0;
}
