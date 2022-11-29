// symtab.
#include <stdio.h>
#include <stdlib.h> //malloc
#include <string.h> //memcpy
#include <elf.h>

#include "global.h"
#include "elf.h"
#include "elfdump.h"
#include "seg.h"
#include "unit.h"

extern sSeg scode;
extern sSeg sdata;

#define FNV_PRIME 16777619
#define FNV_OFFSET_BASIS 2166136261

U32 fnv1a(char*p){
  U32 hash = FNV_OFFSET_BASIS;
  U8 c;
  while((c=*p++)){
    hash = (U32)((hash ^ c) * FNV_PRIME);
  }
  return hash;
}

/*
Ingest sections into code or data segment, assigning base addresses
*/
void unit_ingest(sUnit*pu,sElf* pelf){
  printf("Of interest:\n");
  Elf64_Shdr* shdr = pelf->shdr;  
  for(U32 i=0; i< pelf->shnum; i++,shdr++){
    U64 flags = shdr->sh_flags;
    if(flags & SHF_ALLOC){
      // select code or data segment to append
      sSeg*pseg = (flags & SHF_EXECINSTR) ? &scode : &sdata;
      // src and size of append
      U8* src = (shdr->sh_type == SHT_NOBITS)?
	0 : pelf->buf + shdr->sh_offset;
      seg_align(pseg,shdr->sh_addralign);
      shdr->sh_addr = (U64)seg_append(pseg,src,shdr->sh_size);
      sechead_dump(pelf,i);
    }
  }
}



void unit_prep(sElf* pelf, sUnit* pu){
  //  sSyms* psyms = (sSyms*) malloc(sizeof(psyms));

  U32 cnt = pelf->symnum-1;   //total count of symbols;
  pu->nSyms = cnt;
  
  U64 strings_size = pelf->sh_symtab->sh_size;
  pu->strings = (char*)malloc(strings_size);
  memcpy(pu->strings,pelf->str_sym,strings_size);

  pu->hashes = (U32*)malloc(cnt * 4);

  pu->dats = (sSym*)malloc(cnt * sizeof(sSym));

}
/*
void symbols_ingest(sElf* pelf, sSyms* ps){
  char* strbase = ps->strings;
  
  Elf64_Sym* psym = pelf->psym + ps->cnt;  // Starting from last symbol
  sSymDat* pdat  = ps->dats;
  U32*     phash = ps->hashes;
  
  for(U32 i=1;i<pelf->symnum;i++,psym--,pdat++,phash++){
    *phash = fnv1a(strbase + psym->st_name);
    pdat->ostring = (U32)psym->st_name;
    switch(ELF64_ST_TYPE(psym->st_info)){
      
    }
  }
}
*/
