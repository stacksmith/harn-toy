/*
gcc -c -mmanual-endbr test10.c -O2 -aux-info sym.txt

Dump:
objdump -S test10.o

Relocations:
readelf -W -r test10.o 

Symbols:
readelf -W -s test10.o 


 */

// as simple as it gets
#include <stdio.h>
#include <string.h>
/*
int foo(int i, int j){
  puts("Hello, world\n");
  return i+j;
}
*/

int carp;

char* str = "Hello,world\n";
int bar(int i, int j){
  //  int q = errno;
      puts(str);
      printf("printed %ld characters\n",strlen(str));
      carp = i;
  return j;
} 
