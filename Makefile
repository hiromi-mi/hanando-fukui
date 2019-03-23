CC = clang
CFLAGS = -Wall -Wextra -Wpedantic

objects = main.o
target = hanando

$(target):	$(objects)
	$(CC) $< -o $@ $(LDFLAGS)

test:
	sh test.sh 1 1
	sh test.sh '1+9' 10

clean:
	$(RM) -f $(target) $(objects)

.PHONY:	clean test
