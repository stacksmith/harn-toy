/* Load a single elf object file and fixup */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <elf.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
 
#include "global.h"
#include "util.h"
#include "elf.h"
#include "elfdump.h" 
#include "seg.h"
#include "unit.h"
#include "lib.h"




sElf* pelf;
sSeg scode;
sSeg sdata;

sUnit* puLib;
sUnit** srch_list;

U64 global_symbol_address(char* name){
  sUnit** ppu = srch_list;
  U32 i = units_find_hash(ppu,string_hash(name));
  if(!i){
    printf("Undefined symbol '%s'\n",name);
    exit(1);
  }
//      printf("Undefined %s found: %lx\n",name,i);
  sUnit* pu = *ppu;
  return pu->dats[i].off; // set elf sym value
}



typedef U64 (*fptr)(int,int);


int main(int argc, char **argv){
  seg_alloc(&scode,"SCODE",0x10000000,(void*)0x80000000,
	    PROT_READ|PROT_WRITE|PROT_EXEC);
  seg_alloc(&sdata,"SDATA",0x10000000,(void*)0x40000000,
	    PROT_READ|PROT_WRITE);

  U64 n = 16 * sizeof(sUnit);
  srch_list = (sUnit**)malloc(n);
  memset(srch_list,0,n);

   
  // create bindings for libc
  puLib = lib_make("libc.so.6","libc.txt");
  srch_list[0] = puLib;
  // load an elf file

  sElf* pelf = (sElf*)malloc(sizeof(sElf));
  sUnit* pu = unit_ingest_elf(pelf,argv[1]);
  free(pelf);
  
  srch_list[1]=pu;

  unit_dump(puLib);
  unit_dump(pu);

/*
  fptr bar;
  bar = (fptr)(0x80000016);
  U32 result=0;
    result = (*bar)(1,2);
  printf("result: %d\n",result);

*/
  printf("-------------------\n");
  //  sUnit** u = srch_list;
  //U32 j = (U32)units_find_hash(u,string_hash("bar"));
  //printf("found symbol %p, %X\n",*u,j);
  //unit_dump(*u);
  
  //printf("size of sym is %ld\n",sizeof(sSym));
  //seg_dump(&scode); seg_dump(&sdata);
  
  U32 i = unit_find_hash(pu,string_hash("bar"));
  printf("found %d\n",i);
  if(i){
    fptr entry = (fptr)(U64)(pu->dats[i].off);
    U64 ret = (*entry)(1,2);
    printf("returned: %lx\n",ret);
  }


  return 0;
}
