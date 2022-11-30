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

int main(int argc, char **argv){
  pelf = (sElf*)malloc(sizeof(sElf));
  seg_alloc(&scode,"SCODE",0x10000000,(void*)0x80000000,
	    PROT_READ|PROT_WRITE|PROT_EXEC);
  seg_alloc(&sdata,"SDATA",0x10000000,(void*)0x40000000,
	    PROT_READ|PROT_WRITE);

  U64 n = 16 * sizeof(sUnit);
  srch_list = (sUnit**)malloc(n);
  memset(srch_list,0,n);
  
  puLib = (sUnit*)malloc(sizeof(sUnit));
  void* funs[2]={&puts,&__printf_chk};
  char* names[2]={"puts","__printf_chk"};
  unit_lib(puLib,"lib",2,funs,names);
  seg_dump(&scode); seg_dump(&sdata);

  
  U32 ret = elf_load(pelf,argv[1]);
  printf("Loaded %s (%d bytes)\n",argv[1],ret);
  //  elf_dump(pelf);

  sUnit* pu = (sUnit*)malloc(sizeof(sUnit));
  unit_sections(pu,pelf);
  // reltab_dump(pelf,2);
  seg_dump(&scode); seg_dump(&sdata);

  srch_list[0] = puLib;
  {
  // resolve ELF symbols to actual addresses
    void proc(Elf64_Sym* psym){
      U32 shi = psym->st_shndx; // get section we are referring to
      if(shi){ //for defined symbols, add section base
	psym->st_value += pelf->shdr[shi].sh_addr;
      } else { //for undefined symbols, find!
	char* name = pelf->str_sym + psym->st_name;
	U64 i = units_find_hash(srch_list,string_hash(name));
	if(!i){
	  printf("Undefined symbol '%s'\n",name);
	  exit(1);
	}
	//      printf("Undefined %s found: %lx\n",name,i);
	sUnit* pu = srch_list[i>>32];
	U32 si = (i&0xFFFFFFFF)-1;
	psym->st_value = pu->dats[si].off; // set elf sym value
      }
      //    sym_dump(pelf,psym);
    }
    elf_process_symbols(pelf,proc);
  }
  
  // apply elf relocations to data loaded into unit...
  elf_rels(pelf);
  
  //  symtab_dump(pelf);

  srch_list[1]=pu;

  
  unit_symbols(pu,pelf);
  unit_dump(pu);


  unit_dump(puLib);
  unit_dump(pu);

  
/*
  fptr bar;
  bar = (fptr)(0x80000016);
  U32 result=0;
    result = (*bar)(1,2);
  printf("result: %d\n",result);

*/

  //U64 i = units_find_hash(srch_list,string_hash("puts"));
  //  printf("found symbol %lX\n",i);
  //printf("size of sym is %ld\n",sizeof(sSym));
  seg_dump(&scode); seg_dump(&sdata);
  
  U32 i = unit_find_hash(pu,string_hash("bar"));
  if(i){
    fptr entry = (fptr)(U64)(pu->dats[i-1].off);
    int ret = (*entry)(1,2);
    printf("returned: %d\n",ret);
  }
  
  return 0;
}
