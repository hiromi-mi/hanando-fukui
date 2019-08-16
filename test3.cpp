template<typename T> T add(T a, T b) {
   T c;
   c = a+b;
   return c;
}

int main() {
   printf("int: %d\n", add<int>(234, 123));
   printf("char: %d\n", add<char>(234, 123));
}
