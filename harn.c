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
#include "elfdump.h"
#include "seg.h"
U8* buf;
Elf64_Ehdr* ehdr;
Elf64_Shdr* shdr  ;      // array of section headers
Elf64_Shdr* sh_shstrtab; // strings for section headers
Elf64_Shdr* sh_symtab;   // one and only symbol table

/* -------------------------------------------------------------
   elf_load   Load an ELF object file (via mapping)
 -------------------------------------------------------------*/
U32 elf_load(char* path){
  int fd = open(path, O_RDONLY);
  if(fd<0) {
    printf("Error opening %s\n",path);
    return 0;
  }
  off_t len = lseek(fd, 0, SEEK_END);
  if(fd<0){
    printf("Error seeking end\n");
    return 0;
  }
  buf = (U8*)mmap(0, len, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
  if(!buf) {
    printf("Error mapping'ing %ld bytes\n",len);
    return 0;
  }
  close(fd);
  return len;
}

/* -------------------------------------------------------------
   sh_symtab_set   find the symbol table 

 Elf spec says there is only one.
 -------------------------------------------------------------*/
Elf64_Shdr* sh_symtab_find(){
  for(int i=0;i<ehdr->e_shnum;i++){
    if(shdr[i].sh_type == SHT_SYMTAB){
      return &shdr[i];
    }
  }
  return 0;
}


/* -------------------------------------------------------------
seg_from_section  Append data from an ELF section
---------------------------------------------------------------*/
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

sSeg seg_code;
sSeg seg_data;

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

int main(int argc, char **argv){
  U32 len = elf_load(argv[1]);
  if(!len)
    exit(1);
  
  ehdr = (Elf64_Ehdr*)buf;
  printf("Loaded %s (%d bytes)\n",argv[1],len);
   
  if(ehdr->e_type==ET_REL){
    printf("Relocatable file\n");
  } else {
    fprintf(stderr,"Not a relocatable object file\n");
    exit(1);
  }

  shdr = (Elf64_Shdr*)(buf+ehdr->e_shoff); //array of shdrs
  sh_shstrtab = &shdr[ehdr->e_shstrndx];
  sh_symtab = sh_symtab_find();
  if(!sh_symtab){
    fprintf(stderr,"Error locating symbol table\n");
    exit(1);
  }

  elf_dump();

  seg_alloc(&seg_code,".CODE.",0x100000,(void*)0x80000000,
	    PROT_READ|PROT_WRITE|PROT_EXEC);
  seg_alloc(&seg_data,".DATA.",0x100000,(void*)0x90000000,
	    PROT_READ|PROT_WRITE);

  /* Each loadable section gets copied into code/data segment;
     its segment address is written into the ELF section header */
  printf("\nprocessing segments\n");
  // find all PROGBITS sections
  for(int i=0;i<ehdr->e_shnum;i++) {
    Elf64_Shdr* psh = &shdr[i];
    if((psh->sh_flags & SHF_ALLOC) // allocation requested
       && (psh->sh_size)           // >0 size
       && (psh->sh_type == SHT_PROGBITS)
       && (strcmp(".eh_frame",get_str(sh_shstrtab,psh->sh_name)))
       ){
      if(psh->sh_flags & SHF_EXECINSTR){ // code
	printf("exec--");
	seg_from_section(&seg_code,psh);
      } else { //data
	printf("data--");
	seg_from_section(&seg_data,psh);
      }
      sechead_dump(psh);
    }
  }
  seg_dump(&seg_code);
  seg_dump(&seg_data);
  
  /*
  U8* p = ingest_section(1);
  hd(p,8);
  apply_rel(code,2); 
  hd(p,8);

  fptr f = (fptr)(p+0x16);
  U32 res = (*f)(1,2);
  printf("Result is %d\n",res);
  */
  //  hd(buf + shdr[sh_symtab->sh_link].sh_offset, 8);
  //hd(buf + sh_symtab->sh_offset,8);


  //  secheads_dump();

  //find .text
  printf("OK\n");
  // reltab_dump(2);

  
  return 0;
}
