// test.c

#include "main.h"
extern int *stderr;
extern int *stdout;
#define NULL 0

// See hoc_nyan/test/test.c
#define EXPECT(expr1, expr2) { \
   int e1, e2; \
   e1 = (expr1); \
   e2 = (expr2); \
   if (e1 == e2) { \
      fprintf(stdout, "Line %d: %s: OK %d\n", __LINE__, #expr1, e2); \
   } else { \
      fprintf(stderr, "Line %d: %s: Expr1 %d, Expr2 %d\n", __LINE__, #expr1, e1, e2); \
      retval++; \
   }\
} 0

#define EXPECTVAR(init, expr1, expr2)

int func1(void) {
   int ce;
   ce = -324;
   return ce;
}

char func71(void) {
   char a = -34;
   a -= -45;
   return a;
}

int func72(void) {
   int a = 4;
   a <<= 2;
   a/=2; // 8
   a >>= 2; // 2
   a%=2; // 0
   a-=23; // -23
   a*=3; // -69
   return a;
}

int func10_1(int a) {
   if (a) {
      return 7;
   } else {
      return 42;
   }
   return 21;
}

int func11(int a, int b, int c, int d, int e, int f) {
   // six arguments
   return a+b+c+d+e+f;
}

int func21(int a) {
   // Recursion
   if ( a <= 1) {
      return 1;
   } else {
      return func21(a-1)*a;
   }
}


int main(void) {
   int retval = 0;
   /* test1 */
   EXPECT(1, 1);
   EXPECT(1+9, 10);
   EXPECT(13-9, 4);
   EXPECT(0x1f, 31);
   EXPECT(0X04, 4);
   EXPECT(0Xff, 255);

   /* test2 */
   EXPECT(1*9, 9);
   EXPECT(-1*9, -9);
   EXPECT(-2*-9, 18);
   EXPECT(-2/-9, 0);
   EXPECT((9-11)*34, -68);
   EXPECT(255*2, 510);

   EXPECT(func1(), -324);

   /* test3 */
   EXPECT(3==3, 1);
   EXPECT(3==4, 0);
   EXPECT(-3==4, 0);
   EXPECT(3 != 3+8, 1);
   EXPECT(!(2==2), 0);
   EXPECT(!(-5==2), 1);

   /* test5,6*/
   EXPECT(6%3, 0);
   // C と Python で異なる仕様
   EXPECT(-1%3, -1);
   EXPECT(5%4, 1);
   EXPECT(1^0, 1);
   EXPECT(0^1, 1);
   EXPECT(3^2, 1);
   EXPECT(6|3, 7);
   EXPECT(1|0, 1);
   EXPECT(1&0, 0);
   EXPECT(1<<1, 2);
   EXPECT(1<<0, 1);
   EXPECT(2<<4, 32);
   EXPECT(9>>1, 4);

   /* test7 */
   EXPECT(func71(), 11);
   EXPECT(func72(), -69);

   /* test8 */
   int a = -34;
   EXPECT(++a, -33);
   EXPECT(a++, -33);
   EXPECT(a++, -32);
   EXPECT(a, -31);
   EXPECT(a--, -31);
   EXPECT(a, -32);
   EXPECT(--a, -33);

   /* test9 */
   EXPECT(0>-2, 1);
   EXPECT(-8>12, 0);
   EXPECT(-234232<0, 1);
   EXPECT(43<43, 0);

   /* test10 */
   EXPECT(func10_1(1), 7);
   EXPECT(func10_1(0), 42);
   {
      int a = 0;
      while(a<3) {
         a++;
      }
      EXPECT(a, 3);

      do {
         a++;
      }while(a<0);
      EXPECT(a, 4);
   }

   /* test11 */
   EXPECT(func11(1,-2,3,-4,5,-6), -3);

   /* test12 */
   {
      int x, *y;
      x = 5;
      y=&x;
      EXPECT(*y, 5);
      EXPECT(*y=4, 4);
      EXPECT(x, 4);
   }

   /* test13 */
   {
      int x[10];
      *x=3;
      EXPECT(x[0], 3);
   }

   /* test15 */
   {
      char a;
      EXPECT(sizeof a, 1);
      EXPECT(sizeof(int), 4);
   }

   /* test16 */
   {
      int i;
      for (i=1;i<5;++i) {
         EXPECT(i, 1);
         break;
      }
      EXPECT(i, 1);
      for (i=1;i<5;++i) {
      }
      EXPECT(i, 5);
   }

   EXPECT((3, -4), -4);

   /* test17 */
   /* test18 */
   {
      char a = '\n';
      EXPECT(a, 10);
      puts("TEST18");
   }

   // test20
   EXPECT(-1==-1, 1);
   EXPECT((1==1) || (2==2), 1);
   EXPECT((1==2) || (2==2), 1);
   EXPECT((1==1) || (1==2), 1);
   EXPECT((1==2) || (3==2), 0);
   EXPECT((1==1) && (2==2), 1);
   EXPECT((1==2) && (2==2), 0);
   EXPECT((1==1) && (1==2), 0);
   EXPECT((1==2) && (3==2), 0);
   EXPECT(2 && 1, 1);

   /* test20.5 */
   EXPECT(3 ? 2 : 1, 2);
   EXPECT(0 ? 2 : 1, 1);

   /* test19,21 */
   EXPECT(func21(3), 6);

   /* test22 */
   /* TODO ここの挙動がおかしい */
   EXPECT((char)255+2, 257); // unsigned を仮定
   EXPECT((char)255+(char)2, 1);

   EXPECT(1 <= 2, 1);
   EXPECT(1 <= 1, 1);
   EXPECT(4 <= 1, 0);
   EXPECT(1 >= 2, 0);
   EXPECT(1 >= 1, 1);
   EXPECT(4 >= 1, 1);
   EXPECT(-4 >= 1, 0);
   EXPECT(-4 <= 1, 1);

   /* test23,24 */
   EXPECT(NULL, 0);

   /* test25*/
   EXPECT(((int*)3)+1, 7);
   EXPECT(((int**)3)+1, 11);
   EXPECT(((char*)3)+1, 4);
   EXPECT(((long)3)+1, 4);

   /* test26 */
   {
      int a = 4;
      switch(a) {
         case 4: a=7;
         case 6: a=-8; break;
      }
      EXPECT(a, -8);
      switch(a) {
         case (-7):
            a = 3; break;
         default:
            a = -3411; break;
      }
      EXPECT(a, -3411);
   }

   /* test28*/
   {
      Type a, *b;
      a.argc = -4;
      b=&a;
      EXPECT(b->argc, -4);
      b->argc = 5;
      EXPECT(a.argc, 5);
   }

   // test31
   EXPECT((int)-1 == (long)-1, 1);
   EXPECT((char)0xFFFFF == (int)0xFFFFF, 0);
   EXPECT((int)-1 == (long)4, 0);

   // 空文でレジスタを使い切らないかのテスト
   1;
   2;
   3;
   4;
   5;
   6;
   7;
   8;
   /*
      sh testfdef.sh "int main(){{return 3;}}" 3
      */
   fprintf(stderr, "The number of errors are: %d\n", retval);
   return retval;
}
