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

int main() {
   Test c;
   c.a = 9;
   printf("%d\n", c.Emacs());
   return 0;
}

int Test::Emacs() {
   //printf("From Test::Emacs, %d\n", this->a);
   printf("From Test::Emacs!\n");
   return 8;
}
