#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "global.h"
#include "util.h"
#include "elf.h"
#include "elfdump.h" 
#include "seg.h"
#include "unit.h"
#include "lib.h"
#include "system.h"

extern sSystem sys;

void sys_init(){
  size_t size = (64 * sizeof(sUnit*));
  sys.units = (sUnit**)malloc(size);
  memset(sys.units,0,size);
  sys.nUnits = 0;
}

void sys_add(sUnit* pu){
  sys.units[sys.nUnits++] = pu;
}

sUnit* sys_find_hash(U32 hash,U32* pi){
  sUnit** pulist = sys.units;
  sUnit* pu;
  U32 index;
  while((pu=*pulist++)){
    if((index = unit_find_hash(pu,hash))){
      *pi=index;
      return pu;
    }
  }
  return 0;
}

U64 sys_symbol_address(char* name){
  U32 i;
  sUnit* pu = sys_find_hash(string_hash(name),&i);
  if(!pu){
    fprintf(stderr,"Undefined symbol '%s'\n",name);
    exit(1);
  }
//      printf("Undefined %s found: %lx\n",name,i);
  return pu->dats[i].off; // set elf sym value
}
