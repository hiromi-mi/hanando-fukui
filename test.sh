# test.sh

./hanando $1 > tmp.s
clang tmp.s -o tmp
./tmp
actual="$?"
if [ $actual -ne $2 ]; then
   echo "Error: $2 but $actual"
   exit 1
fi
