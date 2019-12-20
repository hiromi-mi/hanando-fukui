// test.c

#include "main.h"

// See hoc_nyan/test/test.c
#define EXPECT(expr1, expr2) { \
   int e1, e2; \
   e1 = (expr1); \
   e2 = (expr2); \
   if (e1 == e2) { \
      fprintf(stderr, "Line %d: %s: OK\n", __LINE__, #expr1); \
   } else { \
      fprintf(stderr, "Line %d: %s: Expr1 %d, Expr2 %d\n", __LINE__, #expr1, e1, e2); \
      retval++; \
   }\
} 0

int main(void) {
   int retval = 0;
   /* test1 */
   EXPECT(1, 1);
   EXPECT(1+9, 10);
   EXPECT(13-9, 4);
   EXPECT(0x1f, 31);
   EXPECT(0X04, 4);
   EXPECT(0Xff, 255);

   /* test3 */
   EXPECT(3==3, 1);
   EXPECT(3==4, 0);
   EXPECT(-3==4, 0);
   EXPECT(3 != 3+8, 1);
   EXPECT(!(2==2), 0);
   EXPECT(!(-5==2), 1);

   /*
	sh testfdef.sh 'int main() {int; return 2;}' 2
	sh testfdef.sh 'int main() {1; 2; 3; 4; 5; 6; 7; 8; return 2;}' 2
	sh testfdef.sh "int main(){{return 3;}}" 3
        */
   fprintf(stderr, "The number of errors are: %d\n", retval);
   return retval;
}
