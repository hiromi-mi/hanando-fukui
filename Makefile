CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -g -std=c11
LDFLAGS = -lm

#srcs = $(wildcard *.c)
srcs = main.c
objects = $(srcs:.c=.o)
target=hanando

$(target): $(objects)
	$(CC) $(objects) -o $(target) $(LDFLAGS)
$(objects): main.h

self:
	./hanando -r -f main.c > main.s
	$(CC) -g main.s -o main

selfselftest: self
	./main -r -f main.c > main2.s
	$(CC) -g main2.s -o main2
	./main2 -r -f main.c > main3.s
	$(CC) -g main3.s -o main3
	diff -c main2.s main3.s
	diff -c main.s main2.s

test:	test1 test2 test3 test4 \
   test5 test6 test7 test8 test9 \
   test10 test11 test19 test12 test13 test14\
   test15 test16 test17 test18 test20 test21\
   test22 test23 test24 test25 test26 test27\
   test28 test30 test29 test31
	+ make -C samples/

testreg: test1 test2 test3 test4 \
   test5 test6 test7 test8 test9 \
   test10 test11 test12 test13 test14 test15\
   test16 test17 test18 test19 test20\
   test21 test22 test23 test24 test25 test26\
   test27 test28 test29 test30 test31

test1:
	sh testfdef.sh 'int main() {return 1;}' 1 -r
	sh testfdef.sh 'int main() {return 1+9;}' 10 -r
	sh testfdef.sh 'int main() {return 13-9;}' 4 -r

test2:
	sh testfdef.sh 'int main() {return 1*9;}' 9 -r
	sh testfdef.sh 'int main() {return 18/9;}' 2 -r
	sh testfdef.sh 'int main() {return (11-9)*34;}' 68 -r
	sh testfdef.sh 'int main() {int a;a=3;return a;}' 3 -r
	sh testfdef.sh 'int main() {int X;X=3;return X;}' 3 -r
	sh testfdef.sh 'int main() {int ce;ce=3;return ce;}' 3 -r
	sh testfdef.sh 'int main() {int DE;DE=2;return DE;}' 2 -r

test3:
	sh testfdef.sh "int main(){return 3==3;}" 1 -r
	sh testfdef.sh "int main(){return 3==4;}" 0 -r
	sh testfdef.sh "int main(){return 3!=3+8;}" 1 -r
	sh testfdef.sh "int main(){{return 3;}}" 3 -r
	sh testfdef.sh "int main(){return !(2==2);}" 0 -r
	sh testfdef.sh "int main(){return 4==2;}" 0 -r
	sh testfdef.sh "int main(){return !(4==2);}" 1 -r

test4:
	sh testfunccall.sh 'int main(){return func(4);}' OK4 4 -r
	sh testfunccall.sh 'int main(){return foo(4,4);}' 8 0 -r

test5:
	# to avoid printf %%
	sh testfdef.sh 'int main(){return 6 %%3;}' 0 -r
	sh testfdef.sh 'int main(){return 5 %%4;}' 1 -r
	sh testfdef.sh 'int main(){return 1^0;}' 1 -r
	sh testfdef.sh 'int main(){return 1^1;}' 0 -r
	sh testfdef.sh 'int main(){return 3^2;}' 1 -r

test6:
	sh testfdef.sh 'int main(){return 6|3;}' 7 -r
	sh testfdef.sh 'int main(){return 1|0;}' 1 -r
	sh testfdef.sh 'int main(){return 1&0;}' 0 -r
	sh testfdef.sh 'int main(){return 1<<1;}' 2 -r
	sh testfdef.sh 'int main(){return 1>>1;}' 0 -r

test7:
	sh testfdef.sh "int main(){ int a;a=1;a-=1;return a ;}" 0  -r
	sh testfdef.sh "int main(){ int a;a=1;a+=1;return a ;}" 2 -r
	#sh testfdef.sh "int main(){ int a;a=1;a<<=1;a ;}" 2 -r
	sh testfdef.sh "int main(){ int a;a=3;a*=2;return a ;}" 6 -r
	sh testfdef.sh "int main(){ int a;a=6;a/=2;return a ;}" 3 -r
	sh testfdef.sh "int main(){ int a;a=3;a%%=2;return a ;}" 1 -r

test8:
	sh testfdef.sh 'int main(){int a;a=1;++a;return a;}' 2 -r
	sh testfdef.sh 'int main(){int a;a=4;--a;return a;}' 3 -r
	sh testfdef.sh 'int main(){int a;a=1;a++;return a;}' 2 -r
	sh testfdef.sh 'int main(){int a;a=4;a--;return a;}' 3 -r
	sh testfdef.sh 'int main(){int a;a=1;return a++;}' 1 -r
	sh testfdef.sh 'int main(){int a;a=4;return a--;}' 4 -r
	sh testfdef.sh 'int main(){int a;a=1;return ++a;}' 2 -r
	sh testfdef.sh 'int main(){int a;a=4;return --a;}' 3 -r

test9:
	sh testfdef.sh 'int main(){return 2<0;}' 0 -r
	sh testfdef.sh 'int main(){return 0<2;}' 1 -r
	sh testfdef.sh 'int main(){return 2>0;}' 1 -r
	sh testfdef.sh 'int main(){return 0>2;}' 0 -r
	sh testfdef.sh 'int main(){return 3<3;}' 0 -r
	sh testfdef.sh 'int main(){return 2>2;}' 0 -r
	sh testfdef.sh 'int main(){return 2>(-1);}' 1 -r
	sh testfdef.sh 'int main(){return 2<(-1);}' 0 -r

test10:
	sh test.sh "{if(1){return 3;}}" 3 -r
	sh test.sh "{if(1){return 3;}else{return 4;}}" 3 -r
	sh test.sh "{if(0){return 3;}else{return 4;}}" 4 -r
	sh test.sh "int a;a=0;while(a<3){a+=1;} return a;" 3 -r
	sh test.sh "int a;a=0;do{a+=1;}while(a<3); return a;" 3 -r
	sh test.sh "int a=0;do{a+=1;}while(a<0); return a;" 1 -r

test11:
	sh testfdef.sh "int main(){return func()+2;} int func(){return 4;}" 6 -r
	sh testfdef.sh "int main(){return func(1,2,3,4,5);} int func(int a,int b,int c,int d, int e){return a+b+c+d+e;}" 15 -r
	sh testfdef.sh "int main(){return func(1,2,3,4,5,6);} int func(int a,int b,int c,int d, int e, int f){return a+b+c+d+e+f;}" 21 -r
	sh testfdef.sh "int main(){return func()+2;} int func(){return 4;}" 6 -r

test19:
	sh testfdef.sh "int main(){return func(8)+2;} int func(int a){return a-4;}" 6 -r
	sh testfdef.sh "int main(){return func(6+2)+2;} int func(int a){return a-4;}" 6 -r
	sh testfdef.sh "int main(){return func(5); } int func(int a){ if (a==1){return 1;} return 2;}" 2

test21:
	sh testfdef.sh "int main(){return func(3);} int func(int a){if (a==1) {return 1;} else {return func(a-1)*a;}}" 6 -r
	sh testfdef.sh "int main(){return func(3,4);} int func(int a, int b){if (a==1) {return 1;} else {return func(a-1)*a+b-b;}}" 6 -r
	sh testfdef.sh "int func(int a, int b); int main(){return func(3,4);} int func(int a, int b){if (a==1) {return 1;} else {return func(a-1)*a+b-b;}}" 6 -r
 
test12:
	sh testfdef.sh "int main() {int x;int y;x=1;y=2;return x+y;}" 3 -r
	sh testfdef.sh "int main() {int* y;int x;x=3;y=&x;return *y;}" 3 -r

test13:
	sh testfdef.sh "int main(){ int x[10];return 1  ;}" 1 -r
	sh testfdef.sh "int main(){ int x[10];*x=3; return *x ;}" 3 -r
	sh testfdef.sh "int main(){ int x[10];*(x+1)=3;*x=2;return *x;}" 2 -r
	sh testfdef.sh "int main(){ int x[10];x[1]=3; return x[1] ;}" 3 -r
	sh testfdef.sh "int main(){ int x[10];x[0]=2;x[1]=3;return x[1] ;}" 3 -r
	sh testfdef.sh "int main(){ int x[10];x[0]=2;x[1]=3;return x[0] ;}" 2 -r

test14:
	sh testfdef.sh "int a;int main(){a=1;return a;}" 1 -r
	sh testfdef.sh "int a;int main(){return a;}" 0 -r
	sh testfdef.sh "int a=2;int main(){return a;}" 2 -r
	sh testfdef.sh "int a=2;int main(){a=3;return a;}" 3 -r

test15:
	sh testfdef.sh "int main(){char a;return a=1;}" 1 -r
	sh testfdef.sh "int main(){return sizeof 3;}" 4 -r
	sh testfdef.sh "int main(){return sizeof(int);}" 4 -r
	sh testfdef.sh "int main(){int a=1;return a;}" 1 -r

test16:
	sh testfdef.sh "int main(){int i;int j=0;for(i=1;i<5;++i) { j+=i;} return j;}" 10 -r
	sh testfdef.sh "int main(){int i;int j=0;for(i=1;i<5;++i) { break;} return i;}" 1 -r
	sh testfdef.sh "int main(){int i;int j=0;for(i=1;i<5;++i) { j+=2;continue;} return j;}" 8 -r
	sh testfdef.sh "int main(){return (1==2 || 2==3);}" 0 -r
	sh testfdef.sh "int main(){return (1==1);}" 1 -r
	sh testfdef.sh "int main(){return (1==2);}" 0 -r

test20:
	sh testfdef.sh "int main(){return ((1==1) || (2==2));}" 1 -r
	sh testfdef.sh "int main(){return ((1==2) || (2==2));}" 1 -r
	sh testfdef.sh "int main(){return ((1==1) || (1==2));}" 1 -r
	sh testfdef.sh "int main(){return ((1==2) || (3==2));}" 0 -r
	sh testfdef.sh "int main(){return ((1==1) && (2==2));}" 1 -r
	sh testfdef.sh "int main(){return ((1==2) && (2==2));}" 0 -r
	sh testfdef.sh "int main(){return ((1==1) && (1==2));}" 0 -r
	sh testfdef.sh "int main(){return ((1==2) && (3==2));}" 0 -r
	sh testfdef.sh "int main(){return (2 && 1);}" 1 -r

test17:
	sh testfdef.sh 'int main(){char a;return a=1;}' 1 -r

test18:
	sh testfdef.sh "int main(){char a='a';return a;}" 97 -r
	sh testfdef.sh "int main(){char a='\n';return a;}" 10 -r
	sh testfdef.sh 'int main(){return (3,4);}' 4 -r
	sh testfdef.sh "int main(){puts(\"a\");return 0;}" 0 -r
	sh testfdef.sh "int main(){puts(\"Test OK\");return 0;}" 0 -r
	sh testfdef.sh "int main(){printf(\"Test OK\n\");return 0;}" 0 -r

test22:
	sh testfdef.sh "int main(){return 1 <= 2;}" 1 -r
	sh testfdef.sh "int main(){return 1 <= 1;}" 1 -r
	sh testfdef.sh "int main(){return 4 <= 1;}" 0 -r
	sh testfdef.sh "int main(){return 1 >= 2;}" 0 -r
	sh testfdef.sh "int main(){return 1 >= 1;}" 1 -r
	sh testfdef.sh "int main(){return 4 >= 1;}" 1 -r
	sh testfdef.sh "int main(){return -4 >= 1;}" 0 -r
	sh testfdef.sh "int main(){return -4 <= 1;}" 1 -r

test23:
	sh testfdef.sh "int main(){if (2 <= 1) return 1; return 7;}" 7 -r
	sh testfdef.sh "int main(){if (1 <= 2) return 1;}" 1 -r
	sh testfdef.sh "int main(){return NULL;}" 0 -r

test24:
	sh testfdef.sh "int main(){return /* test */ 3;}" 3 -r
	sh testfdef.sh "int main(){return -(-1);}" 1 -r

test25:
	sh testfdef.sh "int main(){return ((int*)3)+1;}" 7 -r
	sh testfdef.sh "int main(){return ((int**)3)+1;}" 11 -r
	sh testfdef.sh "int main(){return ((char*)3)+1;}" 4 -r
	sh testfdef.sh "int main(){return ((int)3)+1;}" 4 -r

test26:
	sh testfdef.sh "int main(){switch(3){case 4: return 5; }}" 5 -r
	sh testfdef.sh "int main(){switch(3){case 4: return 5; case 8: return 7; }}" 5 -r
	sh testfdef.sh "int main(){switch(3){case 4: return 5; case 8: return 7; default: return 10;}}" 10 -r
	sh testfdef.sh "int main(){return -1;}" 255 -r
	sh testfdef.sh "int main(){return +1;}" 1 -r
	sh testfdef.sh 'int main(){return __LINE__;}' 1 -r

test27:
	sh testfdef.sh "typedef struct { int a;int c;} Type; int main(){Type b;b.a = 2;b.c=4;return b.c;}" 4 -r
	sh testfdef.sh "typedef struct { int a;int c;} Type; int main(){Type b;b.a = 2;b.c=4;return b.a;}" 2 -r
	sh testfdef.sh "typedef struct { int a;int c;} Type; int main(){Type b;Type* e; e=&b;e->a = 2;e->c=4;return b.a;}" 2
	sh testfdef.sh "typedef struct { int a;int c;int d;} Type; int main(){Type b;Type* e; e=&b;e->a = 2;e->c=4;b.d=5;return e->d;}" 5

test28:
	sh testfdef.sh "typedef enum {TY_INT, TY_CHAR} TypeConst; int main(){return TY_CHAR;}" 1 -r
	sh testfdef.sh "typedef enum {TY_INT, TY_CHAR} TypeConst; int main(){return TY_INT;}" 0 -r

test29:
	sh testfdef.sh "int func(char* a){ puts(a); return 1; }int main(){return func(\"aaa\");}" 1 -r
	sh testfdef.sh "int main(){char a[3]; a[0]='a'; a[1]='b';a[2]='\\\0';puts(a);return printf(\"%%c%%c\", *a, *(a+1));}" 2 -r
	sh test.sh "int a[3]; a[0]=8; a[1]=2;a[2]=4;puts(a);return printf(\"%d%d\", *a, *(a+1));" 2 -r
	sh test.sh "char a[12]; a[0]='a'; a[1]='c';a[2]='d';a[3]='g';a[4]='h';a[5]='f';a[6]='k';a[7]='p';a[8]='l';a[9]='\0';puts(a);return strcmp(a, \"acdghfkpl\");" 0 -r

test30:
	sh testfdef.sh "int main(){char a;a='h';return ('a'<=a&&a<='z');}" 1 -r
	sh testfdef.sh "int main(){char a;a='h';return ('a'<=a&&a<='z')||('0'<=a&&a<='9')||('A'<=a&&a<='Z')||a=='_';}" 1 -r
	sh testfdef.sh "int main(){char a;a='h';return ('a'<=a&&a<='z')||('0'<=a&&a<='9');}" 1 -r

test31:
	sh testfdef.sh "int main(){return ((int)-1 == (long)(-1));}" 1 -r
	sh testfdef.sh "int main(){return ((int)-1 == (long)(4));}" 0 -r

test32:
	sh testfdef.sh "int main(){ func(1, 2); func(3, 11,12,13); return 0; } int func(...){ int result; va_list ap; va_start(ap, NULL);int cnt; for (cnt = va_arg(ap, int);cnt >0;--cnt) { result = va_arg(ap, int); printf(\"%%d \", result);} putchar('\n'); va_end(ap); return 0;}" 0 -r
	# test32: This should be seen as:
	# 2
	# 11 12 13
	sh testfdef.sh "int main(){ func(1, 2); func(3, 11,12,13); return 0; } int func(int cnt, ...){ int result; va_list ap; va_start(ap, NULL);for (4;cnt >0;--cnt) { result = va_arg(ap, int); printf(\"%%d \", result);} putchar('\n'); va_end(ap); return 0;}" 0 -r
	# test32: This should be seen as:
	# 2
	# 11 12 13
	sh testfdef.sh "int main(){ func(1, \"a\"); func(3, \"b\",\"c\",\"d\"); return 0; } int func(int cnt, ...){ va_list ap; va_start(ap, NULL);for (4;cnt >0;--cnt) { puts(va_arg(ap, char*)); } putchar('\n'); va_end(ap); return 0;}" 0 -r
	# test32: This should be seen as:
	# 2
	# 11 12 13

test33:
	sh testfdef.sh "int main(){ func(\"%%d %%s\n\", 3, \"test\"); return 0;} int func(char* str, ...) { va_list ap; va_start(ap, str); vprintf(str, ap); va_end(ap); return 0;}" 0 -r

test34:
	sh testfdef.sh 'int main(){ int a; float b; b = 2.3; printf("Is 2.3?: %%f\n", (double)b); return 0;}' 0
	echo "This should be 2.3"
	sh testfdef.sh 'int func(float c) {printf("Is 3.4?: %%f\n", (double)c); return 0; } int main(){float b; b = 3.4; func(b); return 0;}' 0
	echo "This should be 3.4"

clean:
	$(RM) -f $(target) $(objects) main.s main2.s main3.s main2 main3

.PHONY:	clean test
