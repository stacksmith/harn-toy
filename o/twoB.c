/*
gcc -c -mmanual-endbr test10.c -O2 -aux-info sym.txt

Dump:
objdump -S test10.o

Relocations:
readelf -W -r test10.o 

Symbols:
readelf -W -s test10.o 


 */

#include <stdio.h>

int sum(int a, int b); // from twoA;

int funB(int a){
  return sum(a,2);
}


