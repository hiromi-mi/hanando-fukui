// $ ./hanando -r -f -cpp test.cpp > test.s && clang -g test.s -o test && ./test

class Test {
   public:
   int a;
   char b;
   static int func();
   static int Vim(int c);
};

class Test2 : Test {
};

int main() {
   Test c;
   c.a = 9;
   printf("%d (Should be 20)\n", Test::func());
   Test::Vim(42);
   Test2 d;
   d.a = 10;
   printf("%d (It must be 10)\n", d.a);
   return 0;
}

static int Test::func() {
   return 20;
}
static int Test::Vim(int c) {
   printf("From Test::Vim %d (Should be 42)\n", c);
   return 1;
}
