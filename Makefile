CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -g

srcs = $(wildcard *.c)
objects = $(srcs:.c=.o)
target=hanando

$(target): $(objects)
	$(CC) $(objects) -o $(target) $(LDFLAGS)
$(objects): main.h
test:	test1 test2 test3 test4 test5 test6 test7 test8

test1:
	sh test.sh '1;' 1
	sh test.sh '1+9;' 10
	sh test.sh '1*9;' 9
	sh test.sh '18/9;' 2

test2:
	sh test.sh '13-9;' 4
	sh test.sh '(11-9)*34;' 68
	sh test.sh 'a=3;a;' 3
	sh test.sh 'ce=3;ce;' 3

test3:
	sh test.sh '3==3;' 1
	sh test.sh '3==4;' 0
	sh test.sh '3!=3+8;' 1
	sh test.sh "{3;}" 3
	sh test.sh "{if(1){3;}}" 3
	#sh test.sh "{while(a|1){a+=1;}}" 3

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
	sh test.sh 'a=1;a-=1;a;' 0
	sh test.sh 'a=1;a+=1;a;' 2
	sh test.sh 'a=3;a*=2;a;' 6
	sh test.sh 'a=6;a/=2;a;' 3
	sh test.sh 'a=3;a%=2;a;' 1

test8:
	sh test.sh 'a=1;++a;a;' 2
	sh test.sh 'a=4;--a;a;' 3

clean:
	$(RM) -f $(target) $(objects)

.PHONY:	clean test
