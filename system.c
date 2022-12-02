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
extern sSeg scode;
extern sSeg sdata;

typedef U32 (*pfsu_iter)(sUnit* pu);

void sys_units_iter(pfsu_iter proc){
  sUnit** pulist = sys.units;
  sUnit* pu;
  while((pu=*pulist++)){
    if ((*proc)(pu)){
      return;
    }
  } 
}
void sys_dump(){
  seg_dump(&scode);
  seg_dump(&sdata);
  printf("%d Units: ",sys.nUnits);
  U32 proc(sUnit* pu){
    printf("%s ",unit_name(pu));
    return 0;
  }
  sys_units_iter(&proc);
  puts(".");
}


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
    return 0;
  }
//      printf("Undefined %s found: %lx\n",name,i);
  return pu->dats[i].off; // set elf sym value
}

sUnit* sys_load_elf(char* path){
  sElf* pelf = elf_new();
  sUnit* pu = unit_ingest_elf1(pelf,path);
  
  U32 unresolved = unit_elf_resolve(pelf,&sys_symbol_address);
  if(unresolved){
    fprintf(stderr,"Undefined symbol '%s'\n",path);
    exit(1);
  }    
  
  unit_ingest_elf2(pu,pelf);
  
  sys_add(pu);
  
  //elf_build_hashlist(pelf);
  //Elf64_Sym* psym = elf_find(pelf,string_hash("puts"));
  //  sym_dump(pelf,psym);
  
  free(pelf);
  return pu;
}

//pElf* elves[16];
//U64 sys_symbol_address

void sys_load_two(char* path1, char* path2){
  sElf* pelf1 = elf_new();
  sElf* pelf2 = elf_new();
  
  sUnit* pu1 = unit_ingest_elf1(pelf1,path1);
  sUnit* pu2 = unit_ingest_elf1(pelf2,path2);

  U32 un1 = unit_elf_resolve(pelf1,&sys_symbol_address);
  U32 un2 = unit_elf_resolve(pelf2,&sys_symbol_address);

  /*  puts("pass2");
  if(un1)
    un1 = unit_elf_resolve(pelf1);
  if(un2)
     un2 = unit_elf_resolve(pelf2);
    
  */
}
