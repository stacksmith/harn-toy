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

void unit_dump(sUnit* pu){
  printf("-----\nUnit\n");
  printf("code: %p, %d\n",scode.base + pu->oCode,pu->szCode);
  printf("data: %p, %d\n",sdata.base + pu->oData,pu->szData);
  printf("%d symbols\n",pu->nSyms);
  
}

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

Of interest are section that need allocation (SHF_ALLOC)..
Executable (SHF_EXECINSTR) sections get placed into the code seg; the
rest go into the data seg.  
* For now, we merge all data - BSS, vars, strings, ro or not
* We section honor alignment
*/ 
void unit_sections(sUnit*pu,sElf* pelf){
  // unit segment positions
  pu->oCode = scode.fill;
  pu->oData = sdata.fill;
 
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
  // update unit sizes
  pu->szCode = scode.fill - pu->oCode;
  pu->szData = sdata.fill - pu->oData;
}

void unit_symbols(sUnit* pu,sElf* pelf){
  //  sSyms* psyms = (sSyms*) malloc(sizeof(psyms));
  U32 cnt = pelf->symnum-1;   //total count of symbols;
  pu->nSyms = cnt;
  
  U64 strings_size = pelf->sh_symtab->sh_size;
  pu->strings = (char*)malloc(strings_size);
  memcpy(pu->strings,pelf->str_sym,strings_size);

  pu->hashes = (U32*)malloc(cnt * 4);
  pu->dats   = (sSym*)malloc(cnt * sizeof(sSym));

  char* strbase  = pu->strings;
  Elf64_Sym* psym = pelf->psym + pu->nSyms;  // Starting from last symbol
  sSym* pdat   = pu->dats;
  U32*  phash  = pu->hashes;
  for(U32 i=1;i<pelf->symnum;i++,psym--,pdat++,phash++){
    *phash = fnv1a(strbase + psym->st_name);
    pdat->ostr = (U32)psym->st_name;
    printf("%s\n",strbase+psym->st_name);
    switch(ELF64_ST_TYPE(psym->st_info)){
      //    case STT_NOTYPE
    }
  }
}

