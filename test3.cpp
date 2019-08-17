/*
$ ./hanando -r -f -cpp test3.cpp > test3.s && gcc -g test3.s -o test3 && ./test3

Output:
   int: 357
   char: 101
*/

#include <stdio.h>
template<typename T> T add(T a, T b) {
   T c;
   c = a+b;
   return c;
}

int main() {
   printf("int: %d\n", add<int>(234, 123));
   printf("char: %d\n", add<char>(234, 123));
}
