// $ ./hanando -r -f -cpp test.cpp > test.s && clang -g test.s -o test && ./test

class Test {
   public:
   int a;
   char b;
   static int func();
   static int Vim(int c);
   int instancefunc();
   int Emacs();
};

int Test::Emacs() {
   printf("From Test::Emacs!\n");
   return 8;
}
static int Test::Vim(int c) {
   printf("From Test::Vim %d\n", c);
   return 1;
}

int main() {
   Test c;
   c.a = 9;
   Test::Vim(32);
   c.Emacs();
   return 0;
}
