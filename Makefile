CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic  -std=c11 -g
LDFLAGS = -lm

#srcs = $(wildcard *.c)
srcs = main.c
objects = $(srcs:.c=.o)
target=hanando

$(target): $(objects)
	$(CC) $(objects) -o $(target) $(LDFLAGS)
$(objects): main.h

self:
	./hanando -f main.c > main.s
	$(CC) -g main.s -o main

selfselftest: self
	./main -f main.c > main2.s
	$(CC) -g main2.s -o main2
	./main2 -f main.c > main3.s
	$(CC) -g main3.s -o main3
	diff -u main2.s main3.s
	diff -u main.s main2.s

ctest:
	clang -E test.c | sed -e "s/^#.*$///" > test.processed.c &&  ./hanando -f test.processed.c > test.s && gcc test.s -o test

test:	test1 test2 test3 test4 \
   test5 test6 test7 test8 test9 \
   test10 test11 test19 test12 test13 test14\
   test15 test16 test17 test18 test20 test21\
   test22 test23 test24 test25 test26 test27\
   test28 test30 test29 test31 test34 test35\
   test36 test32 test33 test37 test38
	+ make -C samples/
	+ make -C cppsamples/

test1:
	sh testfdef.sh 'int main() {return 1;}' 1
	sh testfdef.sh 'int main() {return 1+9;}' 10
	sh testfdef.sh 'int main() {return 13-9;}' 4
	sh testfdef.sh 'int main() {return 0x1F;}' 31
	sh testfdef.sh 'int main() {return 0X04;}' 4
	sh testfdef.sh 'int main() {return 0Xff;}' 255
	sh testfdef.sh 'int main() {int; return 2;}' 2

test2:
	sh testfdef.sh 'int main() {return 1*9;}' 9
	sh testfdef.sh 'int main() {return 18/9;}' 2
	sh testfdef.sh 'int main() {return (11-9)*34;}' 68
	sh testfdef.sh 'int main() {int a;a=3;return a;}' 3
	sh testfdef.sh 'int main() {int X;X=3;return X;}' 3
	sh testfdef.sh 'int main() {int ce;ce=3;return ce;}' 3
	sh testfdef.sh 'int main() {int DE;DE=2;return DE;}' 2

test3:
	sh testfdef.sh "int main(){return 3==3;}" 1
	sh testfdef.sh "int main(){return 3==4;}" 0
	sh testfdef.sh "int main(){return 3!=3+8;}" 1
	sh testfdef.sh "int main(){{return 3;}}" 3
	sh testfdef.sh "int main(){return !(2==2);}" 0
	sh testfdef.sh "int main(){return 4==2;}" 0
	sh testfdef.sh "int main(){return !(4==2);}" 1

test4:
	sh testfunccall.sh 'int main(){return func(4);}' OK4 4
	sh testfunccall.sh 'int main(){return foo(4,4);}' 8 0

test5:
	# to avoid printf %%
	sh testfdef.sh 'int main(){return 6 %%3;}' 0
	sh testfdef.sh 'int main(){return 5 %%4;}' 1
	sh testfdef.sh 'int main(){return 1^0;}' 1
	sh testfdef.sh 'int main(){return 1^1;}' 0
	sh testfdef.sh 'int main(){return 3^2;}' 1

test6:
	sh testfdef.sh 'int main(){return 6|3;}' 7
	sh testfdef.sh 'int main(){return 1|0;}' 1
	sh testfdef.sh 'int main(){return 1&0;}' 0
	sh testfdef.sh 'int main(){return 1<<1;}' 2
	sh testfdef.sh 'int main(){return 1<<0;}' 1
	sh testfdef.sh 'int main(){return 1>>1;}' 0

test7:
	sh testfdef.sh "int main(){ int a;a=1;a-=1;return a ;}" 0
	sh testfdef.sh "int main(){ int a,b;a=2;b=3;return a==b;}" 0
	sh testfdef.sh "int main(){ int a;a=1;a+=1;return a ;}" 2
	sh testfdef.sh "int main(){ int a;a=1;a<<=1;return a;}" 2
	sh testfdef.sh "int main(){ int a;a=2;a>>=1;return a;}" 1
	sh testfdef.sh "int main(){ int a;a=3;a*=2;return a ;}" 6
	sh testfdef.sh "int main(){ int a;a=6;a/=2;return a ;}" 3
	sh testfdef.sh "int main(){ int a;a=3;a%%=2;return a ;}" 1

test8:
	sh testfdef.sh 'int main(){int a;a=1;++a;return a;}' 2
	sh testfdef.sh 'int main(){int a;a=4;--a;return a;}' 3
	sh testfdef.sh 'int main(){int a;a=1;a++;return a;}' 2
	sh testfdef.sh 'int main(){int a;a=4;a--;return a;}' 3
	sh testfdef.sh 'int main(){int a;a=1;return a++;}' 1
	sh testfdef.sh 'int main(){int a;a=4;return a--;}' 4
	sh testfdef.sh 'int main(){int a;a=1;return ++a;}' 2
	sh testfdef.sh 'int main(){int a;a=4;return --a;}' 3

test9:
	sh testfdef.sh 'int main(){return 2<0;}' 0
	sh testfdef.sh 'int main(){return 0<2;}' 1
	sh testfdef.sh 'int main(){return 2>0;}' 1
	sh testfdef.sh 'int main(){return 0>2;}' 0
	sh testfdef.sh 'int main(){return 3<3;}' 0
	sh testfdef.sh 'int main(){return 2>2;}' 0
	sh testfdef.sh 'int main(void){return 2>2;}' 0
	sh testfdef.sh 'int main(){return 2>(-1);}' 1
	sh testfdef.sh 'int main(){return 2<(-1);}' 0

test10:
	sh testfdef.sh "int main(){if(1){return 3;} return 1;}" 3
	sh testfdef.sh "int main(){if(1){return 3;}else{return 4;}}" 3
	sh testfdef.sh "int main(){if(0){return 3;}else{return 4;}}" 4
	sh testfdef.sh "int main(){int a;a=0;while(a<3){a+=1;} return a;}" 3
	sh testfdef.sh "int main(){int a;a=0;do{a+=1;}while(a<3); return a;}" 3
	sh testfdef.sh "int main(){int a=0;do{a+=1;}while(a<0); return a;}" 1

test11:
	sh testfdef.sh "int main(){return func()+2;} int func(){return 4;}" 6
	sh testfdef.sh "int main(){return func(1,2,3,4,5);} int func(int a,int b,int c,int d, int e){return a+b+c+d+e;}" 15
	sh testfdef.sh "int main(){return func(1,2,3,4,5,6);} int func(int a,int b,int c,int d, int e, int f){return a+b+c+d+e+f;}" 21
	sh testfdef.sh "int main(){return func()+2;} int func(){return 4;}" 6

test19:
	sh testfdef.sh "int main(){return func(8)+2;} int func(int a){return a-4;}" 6
	sh testfdef.sh "int main(){return func(6+2)+2;} int func(int a){return a-4;}" 6
	sh testfdef.sh "int main(){return func(5); } int func(int a){ if (a==1){return 1;} return 2;}" 2

test21:
	sh testfdef.sh "int main(){return func(3);} int func(int a){if (a==1) {return 1;} else {return func(a-1)*a;}}" 6
	sh testfdef.sh "int main(){return func(3,4);} int func(int a, int b){if (a==1) {return 1;} else {return func(a-1)*a+b-b;}}" 6
	sh testfdef.sh "int func(int a, int b); int main(){return func(3,4);} int func(int a, int b){if (a==1) {return 1;} else {return func(a-1)*a+b-b;}}" 6

test12:
	sh testfdef.sh "int main() {int x;int y;x=1;y=2;return x+y;}" 3
	sh testfdef.sh "int main() {int* y;int x;x=3;y=&x;return *y;}" 3

test13:
	sh testfdef.sh "int main(){ int x[10];return 1  ;}" 1
	sh testfdef.sh "int main(){ int x[10];*x=3; return *x ;}" 3
	sh testfdef.sh "int main(){ int x[10];*(x+1)=3;*x=2;return *x;}" 2
	sh testfdef.sh "int main(){ int x[10];x[1]=3; return x[1] ;}" 3
	sh testfdef.sh "int main(){ int x[10];x[0]=2;x[1]=3;return x[1] ;}" 3
	sh testfdef.sh "int main(){ int x[10];x[0]=2;x[1]=3;return x[0] ;}" 2

test14:
	sh testfdef.sh "int a;int main(){a=1;return a;}" 1
	sh testfdef.sh "int a;int main(){return a;}" 0
	sh testfdef.sh "int a=2;int main(){return a;}" 2
	sh testfdef.sh "int a=2;int main(){a=3;return a;}" 3

test15:
	sh testfdef.sh "int main(){char a;return a=1;}" 1
	sh testfdef.sh "int main(){return sizeof 3;}" 4
	sh testfdef.sh "int main(){int a; int b; return sizeof a<b;}" 4
	sh testfdef.sh "int main(){float a; float b; return sizeof a==b;}" 4
	sh testfdef.sh "int main(){double a; double b; return sizeof a>b;}" 4
	sh testfdef.sh "int main(){return sizeof(int);}" 4
	sh testfdef.sh "int main(){int a=1;return a;}" 1
	sh testfdef.sh "int a=3*9+5;int main(){return a;}" 32

test16:
	sh testfdef.sh "int main(){int i;int j=0;for(i=1;i<5;++i) { j+=i;} return j;}" 10
	sh testfdef.sh "int main(){int i;int j=0;for(i=1;i<5;++i) { break;} return i;}" 1
	sh testfdef.sh "int main(){int i;int j=0;for(i=1;i<5;++i) { j+=2;continue;} return j;}" 8
	sh testfdef.sh "int main(){return (1==2 || 2==3);}" 0
	sh testfdef.sh "int main(){return (1==1);}" 1
	sh testfdef.sh "int main(){return (1==2);}" 0

test20:
	sh testfdef.sh "int main(){return ((1==1) || (2==2));}" 1
	sh testfdef.sh "int main(){return ((1==2) || (2==2));}" 1
	sh testfdef.sh "int main(){return ((1==1) || (1==2));}" 1
	sh testfdef.sh "int main(){return ((1==2) || (3==2));}" 0
	sh testfdef.sh "int main(){return ((1==1) && (2==2));}" 1
	sh testfdef.sh "int main(){return ((1==2) && (2==2));}" 0
	sh testfdef.sh "int main(){return ((1==1) && (1==2));}" 0
	sh testfdef.sh "int main(){return ((1==2) && (3==2));}" 0
	sh testfdef.sh "int main(){return (2 && 1);}" 1

test17:
	sh testfdef.sh 'int main(){char a;return a=1;}' 1

test18:
	sh testfdef.sh "int main(){char a='a';return a;}" 97
	sh testfdef.sh "int main(){char a='\n';return a;}" 10
	sh testfdef.sh 'int main(){return (3,4);}' 4
	sh testfdef.sh "int main(){puts(\"a\");return 0;}" 0
	sh testfdef.sh "int main(){puts(\"Test OK\");return 0;}" 0
	sh testfdef.sh "int main(){printf(\"Test OK\n\");return 0;}" 0

test22:
	sh testfdef.sh "int main(){return 1 <= 2;}" 1
	sh testfdef.sh "int main(){return 1 <= 1;}" 1
	sh testfdef.sh "int main(){return 4 <= 1;}" 0
	sh testfdef.sh "int main(){return 1 >= 2;}" 0
	sh testfdef.sh "int main(){return 1 >= 1;}" 1
	sh testfdef.sh "int main(){return 4 >= 1;}" 1
	sh testfdef.sh "int main(){return -4 >= 1;}" 0
	sh testfdef.sh "int main(){return -4 <= 1;}" 1

test23:
	sh testfdef.sh "int main(){if (2 <= 1) return 1; return 7;}" 7
	sh testfdef.sh "int main(){if (1 <= 2) return 1;}" 1
	sh testfdef.sh "int main(){return NULL;}" 0

test24:
	sh testfdef.sh "int main(){return /* test */ 3;}" 3
	sh testfdef.sh "int main(){return -(-1);}" 1

test25:
	sh testfdef.sh "int main(){return ((int*)3)+1;}" 7
	sh testfdef.sh "int main(){return ((int**)3)+1;}" 11
	sh testfdef.sh "int main(){return ((char*)3)+1;}" 4
	sh testfdef.sh "int main(){return ((int)3)+1;}" 4

test26:
	sh testfdef.sh "int main(){switch(3){case 4: return 5; }}" 5
	sh testfdef.sh "int main(){switch(3){case 4: return 5; case 8: return 7; }}" 5
	sh testfdef.sh "int main(){switch(3){case 4: return 5; case 8: return 7; default: return 10;}}" 10
	sh testfdef.sh "int main(){return -1;}" 255
	sh testfdef.sh "int main(){return +1;}" 1
	sh testfdef.sh 'int main(){return __LINE__;}' 1

test27:
	sh testfdef.sh "typedef struct { int a;int c;} Type; int main(){Type b;b.a = 2;b.c=4;return b.c;}" 4
	sh testfdef.sh "typedef struct { int a;int c;} Type; int main(){Type b;b.a = 2;b.c=4;return b.a;}" 2
	sh testfdef.sh "typedef struct { int a;int c;} Type; int main(){Type b;Type* e; e=&b;e->a = 2;e->c=4;return b.a;}" 2
	sh testfdef.sh "typedef struct { int a;int c;int d;} Type; int main(){Type b;Type* e; e=&b;e->a = 2;e->c=4;b.d=5;return e->d;}" 5

test28:
	sh testfdef.sh "typedef enum {TY_INT, TY_CHAR} TypeConst; int main(){return TY_CHAR;}" 1
	sh testfdef.sh "typedef enum {TY_INT, TY_CHAR} TypeConst; int main(){return TY_INT;}" 0
	sh testfdef.sh "typedef int MetaInt[13]; int main(){MetaInt c; c[12] = 3;return c[12];}" 3

test29:
	sh testfdef.sh "int func(char* a){ puts(a); return 1; }int main(){return func(\"aaa\");}" 1
	sh testfdef.sh "int main(){char a[3]; a[0]='a'; a[1]='b';a[2]='\\\0';puts(a);return printf(\"%%c%%c\n\", *a, *(a+1));}" 3
	sh testfdef.sh "int main(){int a[3]; a[0]=8; a[1]=2;a[2]=4;puts(a);return printf(\"%%d%%d\n\", *a, *(a+1));}" 3
	sh testfdef.sh "int main(){char a[12]; a[0]='a'; a[1]='c';a[2]='d';a[3]='g';a[4]='h';a[5]='f';a[6]='k';a[7]='p';a[8]='l';a[9]='\\\0';puts(a);return strcmp(a, \"acdghfkpl\");}" 0

test30:
	sh testfdef.sh "int main(){char a;a='h';return ('a'<=a&&a<='z');}" 1
	sh testfdef.sh "int main(){char a;a='h';return ('a'<=a&&a<='z')||('0'<=a&&a<='9')||('A'<=a&&a<='Z')||a=='_';}" 1
	sh testfdef.sh "int main(){char a;a='h';return ('a'<=a&&a<='z')||('0'<=a&&a<='9');}" 1

test31:
	sh testfdef.sh "int main(){return ((int)-1 == (long)(-1));}" 1
	sh testfdef.sh "int main(){return ((int)-1 == (long)(4));}" 0

test32:
	sh testfdef.sh "int main(){ func(1, 2); func(3, 11,12,13); return 0; } int func(...){ int result; va_list ap; va_start(ap, NULL);int cnt; for (cnt = va_arg(ap, int);cnt >0;--cnt) { result = va_arg(ap, int); printf(\"%%d \", result);} putchar('\n'); va_end(ap); return 0;}" 0
	# test32: This should be seen as:
	# 2
	# 11 12 13
	sh testfdef.sh "int main(){ func(1, 2); func(3, 11,12,13); return 0; } int func(int cnt, ...){ int result; va_list ap; va_start(ap, NULL);for (4;cnt >0;--cnt) { result = va_arg(ap, int); printf(\"%%d \", result);} putchar('\n'); va_end(ap); return 0;}" 0
	# test32: This should be seen as:
	# 2
	# 11 12 13
	sh testfdef.sh "int main(){ func(1, \"a\"); func(3, \"b\",\"c\",\"d\"); return 0; } int func(int cnt, ...){ va_list ap; va_start(ap, NULL);for (4;cnt >0;--cnt) { puts(va_arg(ap, char*)); } putchar('\n'); va_end(ap); return 0;}" 0
	# test32: This should be seen as:
	# a
	# b c d

test33:
	sh testfdef.sh "int main(){ func(\"%%d %%s\n\", 3, \"test\"); return 0;} int func(char* str, ...) { va_list ap; va_start(ap, str); vprintf(str, ap); va_end(ap); return 0;}" 0

test34:
	sh testfdef.sh 'int main(){ int a; float b; b = 2.3f; printf("Is 2.3?: %%f\n", (double)b); return 0;}' 0
	sh testfdef.sh 'int main(){ int a; double b; b = 2432342343.5; printf("Is 2432342343.5?: %%f\n", b); return 0;}' 0
	echo "This should be 2.3"
	sh testfdef.sh 'int func(float c) {printf("Is 3.4?: %%f\n", (double)c); return 0; } int main(){float b; b = 3.4; func(b); return 0;}' 0
	echo "This should be 3.4"
	# TODO should add type conversation among TY_INT, TY_FLOAT, TY_DOUBLE
	sh testfdef.sh 'int main(){float b; b = 3.4; printf("It should be 3: %%d\n", (int)b); return 0;}' 0
	sh testfdef.sh 'float strtof(char* basestr, char* ocmpstr); int main(){int a;float b; b = strtof("3.4", NULL); printf("It should be 3.4: %%f\n", (double)b); return 0;}' 0

test35:
	sh testfdef.sh 'int main(){float a,b; a = 8;b=9; printf("It should be 17: %%f\n", (double)(a+b)); return 0;}' 0
	sh testfdef.sh 'int main(){float a,b; a = 8;b=9; printf("It should be -1: %%f\n", (double)(a-b)); return 0;}' 0
	sh testfdef.sh 'int main(){float a,b; a = 11;b=9; printf("It should be 2: %%f\n", (double)(a-b)); return 0;}' 0
	sh testfdef.sh 'int main(){float a,b; a = 8;b=9; printf("It should be 72: %%f\n", (double)(a*b)); return 0;}' 0
	sh testfdef.sh 'int main(){float a,b; a = 12;b=8; printf("It should be 1.5: %%f\n", (double)(a/b)); return 0;}' 0
	sh testfdef.sh 'int main(){double a,b; a = 8;b=9; printf("It should be 17: %%f\n", (double)(a+b)); return 0;}' 0
	sh testfdef.sh 'int main(){double a,b; a = 8;b=9; printf("It should be -1: %%f\n", (double)(a-b)); return 0;}' 0
	sh testfdef.sh 'int main(){double a,b; a = 11;b=9; printf("It should be 2: %%f\n", (double)(a-b)); return 0;}' 0
	sh testfdef.sh 'int main(){double a,b; a = 8;b=9; printf("It should be 72: %%f\n", (double)(a*b)); return 0;}' 0
	sh testfdef.sh 'int main(){double a,b; a = 12;b=8; printf("It should be 1.5: %%f\n", (double)(a/b)); return 0;}' 0

test36:
	sh testfdef.sh 'int main(){int a; float b; b = 3.4; printf("It should be 3: %%d\n", (int)b); return 0;}' 0

test37:
	sh testfdef.sh 'auto func(int a, double b) { return a+b; } int main() { double c = func(3, 4.5); printf("It should be 7.5: %%f\n", c); return 0;}'
	sh testfdef.sh 'auto func(double a, double b) { return a+b; } int main() { double c = func(3.3, 4.5); printf("It should be 7.8: %%f\n", c); return 0;}'

test38:
	sh testfdef.sh 'typedef struct A { int b; } C; int main() { struct A x; return 0; }' 0
clean:
	$(RM) -f $(target) $(objects) main.s main2.s main3.s main2 main3

.PHONY:	clean test
