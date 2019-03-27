# testfunccall.sh

clang foo.c -o foo.o -c
./hanando "func(4);" > tmp.s
clang tmp.s foo.o -o tmp
actual=$(./tmp)
if [ $actual != "OK4" ]; then
   echo "Error: $2 but $actual"
   exit 1
fi
