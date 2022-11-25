/* Load a single self-contained ELF object file and link */

#include <stdio.h>
#include <ctype.h>
#include "global.h"

void* hd_line(void* ptr){
  U8*p=(U8*)ptr;
  printf("%p ",p);
  for(int i=0;i<16;i++){
    printf("%02X ",p[i]);
  }
  for(int i=0;i<16;i++){
    printf("%c",isprint(p[i])?p[i]:128);
  }
  printf("\n");
  return(p+16);
}

void* hd(void*ptr,int lines){
  for(int i=0;i<lines;i++){
    ptr = hd_line(ptr);
  }
  return ptr;
}
