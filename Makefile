CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -g
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
	clang main.s -o main

test:	test1 test2 test3 test4 \
   test5 test6 test7 test8 test9 \
   test10 test11 test19 test12 test13 test14\
   test15 test16 test17 test18 test20 test21\
   test22 test23 test24 test25 test26 test27\
   test28

test1:
	sh test.sh '1;' 1
	sh test.sh '1+9;' 10
	sh test.sh '1*9;' 9
	sh test.sh '18/9;' 2

test2:
	sh test.sh '13-9;' 4
	sh test.sh '(11-9)*34;' 68
	sh test.sh 'int a;a=3;a;' 3
	sh test.sh 'int X;X=3;X;' 3
	sh test.sh 'int ce;ce=3;ce;' 3
	sh test.sh 'int DE;DE=2;DE;' 2

test3:
	sh test.sh '3==3;' 1
	sh test.sh '3==4;' 0
	sh test.sh '3!=3+8;' 1
	sh test.sh "{3;}" 3

test4:
	sh testfunccall.sh 'func(4);' OK4 4
	sh testfunccall.sh 'foo(4,4);' 8 0

test5:
	sh test.sh '6%3;' 0
	sh test.sh '5%4;' 1
	sh test.sh '1^0;' 1
	sh test.sh '1^1;' 0
	sh test.sh '3^2;' 1

test6:
	sh test.sh '6|3;' 7
	sh test.sh '1|0;' 1
	sh test.sh '1&0;' 0
	sh test.sh '1<<1;' 2
	sh test.sh '1>>1;' 0

test7:
	sh test.sh 'int a;a=1;a-=1;a;' 0
	sh test.sh 'int a;a=1;a+=1;a;' 2
	#sh test.sh 'int a;a=1;a<<=1;a;' 2
	sh test.sh 'int a;a=3;a*=2;a;' 6
	sh test.sh 'int a;a=6;a/=2;a;' 3
	sh test.sh 'int a;a=3;a%=2;a;' 1

test8:
	sh test.sh 'int a;a=1;++a;a;' 2
	sh test.sh 'int a;a=4;--a;a;' 3
	sh test.sh 'int a;a=1;a++;a;' 2
	sh test.sh 'int a;a=4;a--;a;' 3
	sh test.sh 'int a;a=1;a++;' 1
	sh test.sh 'int a;a=4;a--;' 4
	sh test.sh 'int a;a=1;++a;' 2
	sh test.sh 'int a;a=4;--a;' 3

test9:
	sh test.sh '2<0;' 0
	sh test.sh '0<2;' 1
	sh test.sh '2>0;' 1
	sh test.sh '0>2;' 0
	sh test.sh '3<3;' 0
	sh test.sh '2>2;' 0
	sh test.sh '2>(-1);' 1
	sh test.sh '2<(-1);' 0

test10:
	sh test.sh "{if(1){3;}}" 3
	sh test.sh "{if(1){3;}else{4;}}" 3
	sh test.sh "{if(0){3;}else{4;}}" 4
	sh test.sh "int a;a=0;while(a<3){a+=1;} return a;" 3
	sh test.sh "int a;a=0;do{a+=1;}while(a<3); return a;" 3
	sh test.sh "int a=0;do{a+=1;}while(a<0); return a;" 1

test11:
	sh testfdef.sh "int main(){func()+2;} int func(){4;}" 6
	sh testfdef.sh "int main(){func(1,2,3,4,5);} int func(int a,int b,int c,int d, int e){a+b+c+d+e;}" 15
	sh testfdef.sh "int main(){func(1,2,3,4,5,6);} int func(int a,int b,int c,int d, int e, int f){a+b+c+d+e+f;}" 21
	sh testfdef.sh "int main(){func()+2;} int func(){return 4;}" 6

test19:
	sh testfdef.sh "int main(){func(8)+2;} int func(int a){return a-4;}" 6
	sh testfdef.sh "int main(){func(6+2)+2;} int func(int a){return a-4;}" 6
	sh testfdef.sh "int main(){func(5);} int func(int a){ if (a==1){1;} 2;}" 2

test21:
	sh testfdef.sh "int main(){func(3);} int func(int a){if (a==1) {1;} else {func(a-1)*a;}}" 6
	sh testfdef.sh "int main(){func(3,4);} int func(int a, int b){if (a==1) {1;} else {return func(a-1)*a+b-b;}}" 6
	sh testfdef.sh "int func(int a, int b); int main(){func(3,4);} int func(int a, int b){if (a==1) {1;} else {return func(a-1)*a+b-b;}}" 6
 
test12:
	sh test.sh "int x;int y;x=1;y=2;x+y;" 3
	sh test.sh "int* y;int x;x=3;y=&x;*y;" 3

test13:
	sh test.sh "int x[10];1;" 1
	sh test.sh "int x[10];*x=3;*x;" 3
	sh test.sh "int x[10];*(x+1)=3;*x=2;*x;" 2
	sh test.sh "int x[10];x[1]=3;x[1];" 3
	sh test.sh "int x[10];x[0]=2;x[1]=3;x[1];" 3
	sh test.sh "int x[10];x[0]=2;x[1]=3;x[0];" 2

test14:
	sh testfdef.sh "int a;int main(){a=1;a;}" 1
	sh testfdef.sh "int a;int main(){a;}" 0
	sh testfdef.sh "int a=2;int main(){a;}" 2
	sh testfdef.sh "int a=2;int main(){a=3;a;}" 3

test15:
	sh test.sh "char a;a=1;" 1
	sh test.sh "sizeof 3;" 4
	sh test.sh "sizeof(int);" 4
	sh test.sh "int a=1;a;" 1

test16:
	sh test.sh "int i;int j=0;for(i=1;i<5;++i) { j+=i;} j;" 10
	sh test.sh "int i;int j=0;for(i=1;i<5;++i) { break;} i;" 1
	sh test.sh "int i;int j=0;for(i=1;i<5;++i) { j+=2;continue;} j;" 8
	sh test.sh "(1==2 || 2==3);" 0
	sh test.sh "(1==1);" 1
	sh test.sh "(1==2);" 0

test20:
	sh test.sh "((1==1) || (2==2));" 1
	sh test.sh "((1==2) || (2==2));" 1
	sh test.sh "((1==1) || (1==2));" 1
	sh test.sh "((1==2) || (3==2));" 0
	sh test.sh "((1==1) && (2==2));" 1
	sh test.sh "((1==2) && (2==2));" 0
	sh test.sh "((1==1) && (1==2));" 0
	sh test.sh "((1==2) && (3==2));" 0
	sh test.sh "(2 && 1);" 1

test17:
	sh test.sh "char a;a=1;" 1

test18:
	sh test.sh "char a='a';a;" 97
	sh test.sh "char a='\n';a;" 10
	# sh test.sh "(3,4);" 4
	sh test.sh "puts(\"a\");0;" 0
	sh test.sh "puts(\"Test OK\");0;" 0
	sh test.sh "printf(\"Test OK\n\");0;" 0

test22:
	sh test.sh "1 <= 2;" 1
	sh test.sh "1 <= 1;" 1
	sh test.sh "4 <= 1;" 0
	sh test.sh "1 >= 2;" 0
	sh test.sh "1 >= 1;" 1
	sh test.sh "4 >= 1;" 1
	sh test.sh "-4 >= 1;" 0
	sh test.sh "-4 <= 1;" 1

test23:
	sh test.sh "if (2 <= 1) 1; 7;" 7
	sh test.sh "if (1 <= 2) 1;" 1
	sh test.sh "NULL;" 0

test24:
	sh test.sh "/* test */ 3;" 3
	sh test.sh "-(-1);" 1

test25:
	sh test.sh "((int*)3)+1;" 7
	sh test.sh "((int)3)+1;" 4

test26:
	sh test.sh "switch(3){case 4: return 5; }" 5
	sh test.sh "switch(3){case 4: return 5; case 8: return 7; }" 5
	sh test.sh "switch(3){case 4: return 5; case 8: return 7; default: return 10;}" 10
	sh test.sh "-1;" 255
	sh test.sh "+1;" 1
	sh test.sh '__LINE__;' 1

test27:
	sh testfdef.sh "typedef struct { int a;int c;} Type; int main(){Type b;b.a = 2;b.c=4;b.c;}" 4
	sh testfdef.sh "typedef struct { int a;int c;} Type; int main(){Type b;b.a = 2;b.c=4;b.a;}" 2
	sh testfdef.sh "typedef struct { int a;int c;} Type; int main(){Type b;Type* e; e=&b;e->a = 2;e->c=4;b.a;}" 2
	sh testfdef.sh "typedef struct { int a;int c;int d;} Type; int main(){Type b;Type* e; e=&b;e->a = 2;e->c=4;b.d=5;e->d;}" 5

test28:
	sh testfdef.sh "typedef enum {TY_INT, TY_CHAR} TypeConst; int main(){TY_CHAR;}" 1
	sh testfdef.sh "typedef enum {TY_INT, TY_CHAR} TypeConst; int main(){TY_INT;}" 0

test29:
	sh testfdef.sh "int func(char* a){ puts(a); return 1; }int main(){func(\"aaa\");}" 1
	sh test.sh "char a[3]; a[0]='a'; a[1]='b';a[2]='\0';puts(a);printf(\"%c%c\", *a, *(a+1));" 2
	sh test.sh "int a[3]; a[0]=8; a[1]=2;a[2]=4;puts(a);printf(\"%d%d\", *a, *(a+1));" 2
	sh test.sh "char a[12]; a[0]='a'; a[1]='c';a[2]='d';a[3]='g';a[4]='h';a[5]='f';a[6]='k';a[7]='p';a[8]='l';a[9]='\0';puts(a);strcmp(a, \"acdghfkpl\");" 0

clean:
	$(RM) -f $(target) $(objects)

.PHONY:	clean test
