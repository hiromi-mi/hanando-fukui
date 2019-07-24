# testfunccall.sh

clang foo.c -o foo.o -c
./hanando $4 "$(printf "$1")" > tmp.s
clang tmp.s foo.o -o tmp
actual=$(./tmp)
retval=$?
if [ "$actual" != $2 ]; then
   echo "Error: $2 but $actual"
   exit 1
fi
if [ $retval != $3 ]; then
   echo "Error: Retren $3 but $retval"
   exit 1
fi
