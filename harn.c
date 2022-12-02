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
#include "system.h"




sElf* pelf;
sSeg scode;
sSeg sdata;

sSystem sys;

typedef U64 (*fptr)(int,int);


int main(int argc, char **argv){
  sys_init(&sys);
  
  seg_alloc(&scode,"SCODE",0x10000000,(void*)0x80000000,
	    PROT_READ|PROT_WRITE|PROT_EXEC);
  seg_alloc(&sdata,"SDATA",0x10000000,(void*)0x40000000,
	    PROT_READ|PROT_WRITE);

  // create bindings for libc
  sys_add(lib_make("libc.so.6","libc.txt"));
 
  // load an elf file

  sElf* pelf = elf_new();
  sys_add(unit_ingest_elf(pelf,argv[1]));
  free(pelf);

  //  unit_dump(puLib);
  //unit_dump(pu);

  printf("-------------------\n");

  fptr entry = (fptr)sys_symbol_address("bar");
  printf("found %p\n",entry);
  if(entry){
    //    fptr entry = (fptr)(U64)(pu->dats[i].off);
    U64 ret = (*entry)(1,2);
    printf("returned: %lx\n",ret);
  }

  //printf("wprintf is at %x\n",sys_symbol_address("wprintf"));

  return 0;
}
