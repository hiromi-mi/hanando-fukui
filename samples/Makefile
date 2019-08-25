CC = ../hanando
test:	test3 test8 test11 test13 test14 test31 test17 test35 test41
	$(CC) -r -f 9.c > 9.s
	gcc 9.s -o 9
	./9
	$(CC) -r -f 10.c > 10.s
	gcc 10.s -o 10
	./10 "aaa"

test11:
	$(CC) -r -f 11.c > 11.s
	gcc -g -S -masm=intel 12.c -o 12.s
	gcc -g 12.s 11.s -o 11
	./11
test13:
	$(CC) -r -f 13.c > 13.s
	gcc 13.s -o 13
	./13

test14:
	$(CC) -r -f 14.c > 14.s
	gcc 14.s -o 14
	./14

test3:
	$(CC) -r -f 3.c > 3.s
	gcc -S 4.c -masm=intel
	gcc 3.s 4.s -o 3
	./3
test8:
	$(CC) -r -f 8.c > 8.s
	gcc 8.s -o 8
	./8
test31:
	$(CC) -r -f 31.c > 31.s
	gcc 31.s -o 31 
	./31
test18:
	$(CC) -r -f 18.c > 18.s
	gcc -g 18.s 19.c -o 18
	./18

test17:
	$(CC) -r -f 17.c > 17.s
	gcc -g 17.s -o 17
	./17

test41:
	$(CC) -r -f 41.c > 41.s
	gcc -g 41.s -o 41
	./41

test35:
	./35.bash

test36:
	./36.bash
clean:
	$(RM) -f *.s

.PHONY:	test3 test8 test11 test13 test14 test31 clean test17 test41
