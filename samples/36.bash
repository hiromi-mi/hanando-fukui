#!/bin/bash

gcc -g 36.c -o 36_gcc
../hanando -f 36.c > 36.s
gcc -g 36.s -o 36_hanando
gcc_result=$(./36_gcc)
hanando_result=$(./36_hanando)
if test "${gcc_result}" != "${hanando_result}"
then
   echo $gcc_result
   echo "Error: samples/36 does not match"
   echo $hanando_result
   return 1
fi
