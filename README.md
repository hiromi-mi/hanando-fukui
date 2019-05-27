# hanando-fukui !EXPERIMENTAL VERSION!
Self-hostable C Compiler Prototype: 

**Currently This cannot be compiled yet. To consult hanando-fukui quickly, use Version 1.1 (`git checkout v1.1`) instead.**

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
$ make self
$ make test
$ make selfselftest
```
