test:
	../hanando -f 3.c > 3.s
	gcc -S 4.c -masm=intel
	gcc 3.s 4.s -o 3
	./3
	../hanando -f 8.c > 8.s
	gcc 8.s -o 8
	./8
