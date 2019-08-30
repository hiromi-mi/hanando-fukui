// $ ./hanando -r -f -cpp test.cpp > test.s && clang -g test.s -o test && ./test

class Test {
   int a;
   protected:
   int f;
   public:
   char b;
   static int Vim(int c);
   void SetA(int new_a);
   int Emacs();
};

class Test2 : Test {
   int SetF(int f);
};

void Test::SetA(int new_a) {
   this->a = new_a;
}

int Test2::SetF(int f) {
   //return this->a; // this should be error
   this->f = f;
   return this->f;
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
   //c.a = 9;
   c.SetA(9);
   //Test::Vim(32);
   c.Emacs();
   return 0;
}
