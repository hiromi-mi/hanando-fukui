CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -g

objects = main.o
target = hanando

$(target):	$(objects)
	$(CC) $< -o $@ $(LDFLAGS)

test:
	sh test.sh '1;' 1
	sh test.sh '1+9;' 10
	sh test.sh '1*9;' 9
	sh test.sh '18/9;' 2
	sh test.sh '13-9;' 4
	sh test.sh '(11-9)*34;' 68
	sh test.sh 'a=3;a;' 3

clean:
	$(RM) -f $(target) $(objects)

.PHONY:	clean test
