# testfunccall.sh

clang foo.c -o foo.o -c
./hanando "func(4);" > tmp.s
clang tmp.s foo.o -o tmp
actual=$(./tmp)
retval=$?
if [ $actual != "OK4" ]; then
   echo "Error: OK4 but $actual"
   exit 1
fi
if [ $retval != 4 ]; then
   echo "Error: Retren 4 but $retval"
   exit 1
fi
