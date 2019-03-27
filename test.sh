# test.sh

tmps=$(mktemp).s
# Not safe for exception trapping.
./hanando $1 > $tmps
clang $tmps -o $tmps.run
$tmps.run
actual="$?"
if [ $actual -ne $2 ]; then
   echo "Error: $2 but $actual"
   exit 1
fi
rm -f $tmps $tmps.run
