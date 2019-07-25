# hanando-fukui
Self-hostable C Compiler Prototype: Version 2.0 (Register-based)

## How to install
By just typing `$ make` will compile as `./hanando`.

Then, you can use hanando and output assembly to standard output.
```
$ ./hanando -r "int main() {printf(\"Hello World!\");}"
```
Using escaping of quotation is needed to run inside bash.

Alternatively, you can specify and compile a C file.
```
$ ./hanando -r -f bar.c
```
After outputting of assembly, you can compile to an executable with gcc or clang:
```
$ ./hanando -r -f bar.c > bar.s
$ gcc bar.s -o bar
$ ./bar
```

## How to compile self-recursively
```
$ make self
$ make selfselftest
```
