// $ ./hanando -r -f -cpp test.cpp > test.s && clang -g test.s -o test && ./test

class Test {
   public:
   int a;
   char b;
   static int func();
   static int Vim(int c);
   int instancefunc();
};

int main() {
   Test c;
   c.a = 9;
   printf("%d\n", Test::func());
   Test::Vim(42);
   return 0;
}

static int Test::func() {
   return 20;
}
static int Test::Vim(int c) {
   printf("From Test::Vim %d\n", c);
   return 1;
}
