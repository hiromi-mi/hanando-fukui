CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -g

srcs = $(wildcard *.c)
objects = $(srcs:.c=.o)
target=hanando

$(target): $(objects)
	$(CC) $(objects) -o $(target) $(LDFLAGS)
$(objects): main.h
test:	test1 test2 test3 test4 \
   test5 test6 test7 test8 test9 \
   test10 test11 test12 test13 test14

test1:
	sh test.sh '1;' 1
	sh test.sh '1+9;' 10
	sh test.sh '1*9;' 9
	sh test.sh '18/9;' 2

test2:
	sh test.sh '13-9;' 4
	sh test.sh '(11-9)*34;' 68
	sh test.sh 'int a;a=3;a;' 3
	sh test.sh 'int ce;ce=3;ce;' 3

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

test9:
	sh test.sh '2<0;' 0
	sh test.sh '0<2;' 1
	sh test.sh '2>0;' 1
	sh test.sh '0>2;' 0

test10:
	sh test.sh "{if(1){3;}}" 3
	sh test.sh "{if(1){3;}else{4;}}" 3
	sh test.sh "{if(0){3;}else{4;}}" 4
	sh test.sh "int a;a=0;while(a<3){a+=1;} return a;" 3

test11:
	sh testfdef.sh "int main(){func()+2;} int func(){4;}" 6
	sh testfdef.sh "int main(){func()+2;} int func(){return 4;}" 6
	sh testfdef.sh "int main(){func(8)+2;} int func(int a){return a-4;}" 6
	sh testfdef.sh "int main(){func(5);} int func(int a){ if (a==1){1;} 2;}" 2
	sh testfdef.sh "int main(){func(3);} int func(int a){if (a==1) {1;} else {func(a-1)*a;}}" 6
	sh testfdef.sh "int main(){func(3,4);} int func(int a, int b){if (a==1) {1;} else {return func(a-1)*a+b-b;}}" 6
 
test12:
	sh test.sh "int x;int y;x=1;y=2;x+y;" 3
	sh test.sh "int* y;int x;x=3;y=&x;*y;" 3

test13:
	sh test.sh "int x[10];1;" 1
	sh test.sh "int x[10];*x=3;*x;" 3
	sh test.sh "int x[10];*(x+1)=3;*x=2;*x;" 2
	sh test.sh "int x[10];x[1]=3;x[1];" 3

test14:
	sh testfdef.sh "int a;int main(){a=1;a;}" 1


clean:
	$(RM) -f $(target) $(objects)

.PHONY:	clean test
