#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h> //malloc
#include <sys/mman.h>


#include "global.h"
#include "util.h"
#include "elf.h"
#include "elfdump.h"
#include "unit.h"
#include "system.h"
#include "elvs.h"

/* for multi-elf loads, we need to try to resolve undefined symbols.
  For that, we build a hashlist of all GLOBAL OBJECTs and FUNCs, 
  resolving to proper sElf and Syms.

*/
U64 sys_symbol_address(char* name);

void elvs_init(sElvs* pm, U32 cnt,char**paths){
  pm->nelfs = cnt;
  pm->pelfs =  (sElf**)malloc((cnt+1) * sizeof(void*));
  pm->ppu    = (sUnit**)malloc((cnt+1) * sizeof(void*));
  pm->hashes = (U32*)malloc(0x10000*4);//64k hashes
  pm->ielf   = (U8*)malloc(0x10000);
  pm->isym   = (U16*)malloc(0x10000*2);

  for(U32 i=0;i<cnt;i++){
    sElf* pelf = elf_new();
    pm->pelfs[i] = pelf;
    pm->ppu[i] = unit_ingest_elf1(pelf,paths[i]);
  }
  pm->pelfs[cnt]=0;
  pm->ppu[cnt]=0;
  
  U32* phash = pm->hashes;
  U8*  pielf  = pm->ielf;
  U16* pisym  = pm->isym;
  U8   ielf   = 0;  // elf index counter
  
  
  sElf**ppelf = pm->pelfs;
  sElf* pelf;

  U32 nsyms = 0;
    // select only visible, defined symbols  
  void proc(Elf64_Sym* psym,U32 i){
    if(psym->st_shndx && ELF64_ST_BIND(psym->st_info)){
      printf("%d %d adding %s\n",ielf,i,(pelf->str_sym + psym->st_name));
      *phash++ = string_hash(pelf->str_sym + psym->st_name);
      *pielf++ = ielf;
      *pisym++ = i;
      nsyms++;
    }
  }

  while((pelf=*ppelf++)){
    elf_process_symbols(pelf,proc);
    ielf++;
  }
  *phash = 0;
  *pielf = 0;
  *pisym = 0;
  pm->nsyms = nsyms;
  




}

//void melf_make_hashlist(sElf** melf){


void elvs_dump(sElvs* pm){
  printf("sElvs with %d elfs and %d symbols\n",pm->nelfs,pm->nsyms);
  U32* phash = pm->hashes;
  U8*  pielf = pm->ielf;
  U16* pisym = pm->isym;

  U32 hash;
  while((hash = *phash++)){
    U32 ielf = *pielf++;
    U32 isym = *pisym++;
    sElf* pelf = pm->pelfs[ielf];
    Elf64_Sym* psym = pelf->psym + isym;
    sym_dump(pelf,psym);
  }
}

U64 elvs_find(sElvs*pm, char* name){
  U32 needle = string_hash(name);
  
  U32* phash = pm->hashes;
  U8*  pielf = pm->ielf;
  U16* pisym = pm->isym;

  U32 hash;
  while((hash = *phash++)){
    U32 ielf = *pielf++;
    U32 isym = *pisym++;
    if(needle==hash){
      sElf* pelf = pm->pelfs[ielf];
      Elf64_Sym* psym = pelf->psym + isym;
      return psym->st_value;
    }
  }
  return 0L;
}
U64 elvs_resolve_symbols(sElvs*pm){
  sElf** pelfs = pm->pelfs;
  sElf* pelf;
  U32 unresolved = 0;
  while((pelf=*pelfs++)){
    unresolved += elf_resolve_symbols(pelf,&sys_symbol_address);
  }
  return unresolved;
}
U64 elvs_resolve_undefs(sElvs*pm){
  sElf** pelfs = pm->pelfs;
  sElf* pelf;
  U32 unresolved = 0;
  U64 resolver(char* symname){
    return elvs_find(pm,symname);
  }
  while((pelf=*pelfs++)){
    unresolved += elf_resolve_undefs(pelf,resolver);
  }
  return unresolved;
}

void elvs_step2(sElvs*pm){
  sElf** pelfs = pm->pelfs;
  sElf* pelf;
  sUnit** ppu = pm->ppu;
  sUnit* pu;
  while((pelf=*pelfs++)){
    pu=*ppu++;
    unit_ingest_elf2(pu,pelf);
    sys_add(pu);
  }

}
