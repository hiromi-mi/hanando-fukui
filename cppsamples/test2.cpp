// $ ./hanando -r -f -cpp test.cpp > test.s && clang -g test.s -o test && ./test

class Test {
   int a;
   public:
   char b;
   static int Vim(int c);
   void SetA(int new_a);
   int Emacs();
};

void Test::SetA(int new_a) {
   this->a = new_a;
}

int Test::Emacs() {
   printf("From Test::Emacs! %d (Should be 9)\n", this->a);
   return 8;
}
static int Test::Vim(int c) {
   printf("From Test::Vim %d\n", c);
   return 1;
}

int main() {
   Test c;
   // Should ERROR
   c.a = 9;
   c.SetA(9);
   //Test::Vim(32);
   c.Emacs();
   return 0;
}
