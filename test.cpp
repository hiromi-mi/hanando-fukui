class Test {
   public:
   int a;
   char b;
   static int func();
};

int main() {
   Test c;
   c.a = 9;
   printf("%d\n", Test::func());
   return 0;
}

static int Test::func() {
   return 20;
}
