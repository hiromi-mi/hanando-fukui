# hanando-fukui
Self-hostable C(+something) Compiler Prototype: Version 3.1.5 (2020/01/04)

*Japanese README is avaliable on README.ja.md*

## How to install
By just typing `$ make` will compile as `./hanando`.

Then, you can use hanando and output assembly to standard output.
```
$ ./hanando "int main() {printf(\"Hello World!\");}"
```
Using escaping of quotation is needed to run inside bash.

Alternatively, you can specify and compile a C file.
```
$ ./hanando -f bar.c
```
After outputting of assembly, you can compile to an executable with gcc or clang:
```
$ ./hanando -f bar.c > bar.s
$ gcc bar.s -o bar
$ ./bar
```

## How to compile self-recursively
```
$ make selftest
```
