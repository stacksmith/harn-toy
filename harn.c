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
#include "hexdump.h"
#include "elf.h"
#include "elfdump.h"
#include "seg.h"
#include "unit.h"

sElf* pelf;
sSeg scode;
sSeg sdata;

/*
void seg_from_section(sSeg* pseg,Elf64_Shdr* psec){
  U32 align = (U32)psec->sh_addralign;
  int rem  = ((U64)pseg->fillptr % align);
  if(rem) {
    seg_append(pseg,0,align-rem);
    printf("Inserted pad of %d bytes\n",align-rem);
  }  
  // src is either section, or, if NOBITS, 0
  U8* src = (psec->sh_type==SHT_NOBITS) ? 0 : buf + psec->sh_offset;
   U8* dest = seg_append(pseg, src, psec->sh_size);
  psec->sh_addr = (Elf64_Addr)dest;
}


void apply_rel(U8* code,U32 ri){
  Elf64_Shdr* psh = &shdr[ri];
   Elf64_Shdr* pshstr = &shdr[sh_symtab->sh_link]; //sh of strings
  Elf64_Sym*  syms = (Elf64_Sym*)(buf + sh_symtab->sh_offset);
  Elf64_Rela* prel = (Elf64_Rela*) (buf + psh->sh_offset);
  int cnt = psh->sh_size / psh->sh_entsize;
  for(int i = 0;i < cnt; i++,prel++){
    rel_dump(prel);
    Elf64_Sym* psym = &syms[ELF64_R_SYM(prel->r_info)];
    U32 reltype = ELF64_R_TYPE(prel->r_info);
    sym_dump(pshstr,psym);
    switch(reltype){
    case 4: //plt32
      U64 p = ((U64)code) + prel->r_offset;
      S64 a = prel->r_addend;
      U64 s = ((U64)code) + psym->st_value;
      U32 fixup = (U32)(s+a-p);
      printf("Fixup: P:%lx A:%ld S:%lx S+A-P: %08x\n",p,a,s,fixup);
      *((U32*)p) = fixup;
      break;
    default:
      fprintf(stderr,"apply_rel: unknown rel type %d\n",reltype);
      exit(-1);
    }
  }
}
*/


/* create a unit with our library linkage */
sUnit* puLib;
sUnit** srch_list;

typedef int (*fptr)(int,int);

// global symbol-address resolver
//
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

void make_lib(void){
  puLib = (sUnit*)malloc(sizeof(sUnit));
  void* funs[2]={&puts,&__printf_chk};
  char* names[2]={"puts","__printf_chk"};
  unit_lib(puLib,"lib",2,funs,names);
  srch_list[0] = puLib;
}

void ingest_elf(char* path,sElf* pelf, sUnit* pu){
  // seg_dump(&scode); seg_dump(&sdata);
  elf_load(pelf,path);
  //  printf("Loaded %s (%d bytes)\n",argv[1],ret);
  //  elf_dump(pelf);
  unit_sections_from_elf(pu,pelf);
  elf_resolve_symbols(pelf);
  elf_apply_rels(pelf);
  unit_symbols_from_elf(pu,pelf);
}

int main(int argc, char **argv){
  seg_alloc(&scode,"SCODE",0x10000000,(void*)0x80000000,
	    PROT_READ|PROT_WRITE|PROT_EXEC);
  seg_alloc(&sdata,"SDATA",0x10000000,(void*)0x40000000,
	    PROT_READ|PROT_WRITE);

  U64 n = 16 * sizeof(sUnit);
  srch_list = (sUnit**)malloc(n);
  memset(srch_list,0,n);

  
  make_lib();
  sUnit* pu = (sUnit*)malloc(sizeof(sUnit));
  pelf = (sElf*)malloc(sizeof(sElf));
  ingest_elf(argv[1],pelf,pu);

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
    int ret = (*entry)(1,2);
    printf("returned: %d\n",ret);
  }


  return 0;
}
