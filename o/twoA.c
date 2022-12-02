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
int A;

int funB(int b);

int sum(int a, int b){
  return a+b;
}

int funA(int a){
  return a + funB(a);  // a + a+2
}


