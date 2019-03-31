CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -g

objects = main.o util.o
target = hanando

$(target):	$(objects)
	$(CC) $? -o $@ $(LDFLAGS)

test:	test1 test2 test3 test4

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

test4:
	sh testfunccall.sh 'func(4);' OK4 4
	sh testfunccall.sh 'foo(4,4);' 8 0

clean:
	$(RM) -f $(target) $(objects)

.PHONY:	clean test
