CC = ../main
test:
	../main -f 3.c > 3.s
	gcc -S 4.c -masm=intel
	gcc 3.s 4.s -o 3
	./3
	../main -f 8.c > 8.s
	gcc 8.s -o 8
	./8
	../main -f 9.c > 9.s
	gcc 9.s -o 9
	./9
	../main -f 10.c > 10.s
	gcc 10.s -o 10
	./10 "aaa"
	../main -f 11.c > 11.s
	gcc -g -S -masm=intel 12.c -o 12.s
	gcc -g 12.s 11.s -o 11
	./11
	../main -f 13.c > 13.s
	gcc 13.s -o 13
	./13
	../main -f 14.c > 14.s
	gcc 14.s -o 14
	./14
