# test.sh

tmps=$(mktemp --tmpdir XXXXXX.s)
tmprun=$(mktemp --tmpdir XXXXXX.run)
# Not safe for exception trapping.
naiyou=$1
naiyou2="int main(){$naiyou}"
echo $naiyou2
./hanando $3 "$naiyou2" > $tmps
clang $tmps -g -o $tmprun
$tmprun
actual="$?"
if [ $actual -ne $2 ]; then
   echo "Error: $2 but $actual on $tmps and $1"
   exit 1
fi
rm -f $tmps $tmprun
