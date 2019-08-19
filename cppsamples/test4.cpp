typedef struct {
   long num_val;
   char *input;
   int pos;
   int type_size; // to treat 'a' as char. only used in TK_NUM
   int pline;
} Token;

#include <stdio.h>

template<typename T, typename U> T add(T a, U b) {
   T c;
   c = a+b;
   return c;
}
template<typename V> int get(V a) {
   return a->pline;
}

int main() {
   printf("int: %d\n", add<int, char>(234, 123));
   printf("char: %d\n", add<char, int>(234, 123));
   Token d;
   d.pline = 8;
   printf("token: %d\n", get<Token*>(&d));
   return 0;
}
