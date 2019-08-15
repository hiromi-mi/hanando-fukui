// test of function pointer

int func() {
   return  9;
}

int main() {
   //printf("Call Directly: %d\n", func());
   int (*x)();
   x = func;
   printf("Call with Pointer: %d\n", x());
   return 0;
}
