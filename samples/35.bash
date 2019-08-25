#!/bin/bash

gcc 35.c -o 35_gcc
../hanando -f 35.c > 35.s
gcc 35.s -o 35_hanando
gcc_result=$(./35_gcc)
hanando_result=$(./35_hanando)
if test "${gcc_result}" != "${hanando_result}"
then
   echo $gcc_result
   echo "Error: samples/35 does not match"
   echo $hanando_result
   return 1
fi
