typedef struct {
   long num_val;
   char *input;
   int pos;
   int type_size; // to treat 'a' as char. only used in TK_NUM
   int pline;
} Token;

template<typename T> T add(T a, T b) {
   T c;
   c = a+b;
   return c;
}
template<typename U> U get(U a) {
   return a->pline;
}

int main() {
   printf("int: %d\n", add<int>(234, 123));
   printf("char: %d\n", add<char>(234, 123));
   Token d;
   d.pline = 8;
   printf("token: %d\n", get<Token*>(&d));
   return 0;
}
