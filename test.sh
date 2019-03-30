# test.sh

tmps=$(mktemp --tmpdir XXXXXX.s)
tmprun=$(mktemp --tmpdir XXXXXX.run)
# Not safe for exception trapping.
./hanando $1 > $tmps
clang $tmps -o $tmprun
$tmprun
actual="$?"
if [ $actual -ne $2 ]; then
   echo "Error: $2 but $actual"
   exit 1
fi
rm -f $tmps $tmprun
