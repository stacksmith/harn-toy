#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <elf.h>
#include "global.h"
#include "elf.h"
#include "elfdump.h"

/* -------------------------------------------------------------
   elf_load   Load an ELF object file (via mapping)
 -------------------------------------------------------------*/
U32 elf_load(sElf* pelf,char* path){
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
  pelf->buf = (U8*)mmap(0, len, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
  if(!pelf->buf) {
    printf("Error mapping'ing %ld bytes\n",len);
    return 0;
  }
  close(fd);
  // section header array
  pelf->shdr = (Elf64_Shdr*)(pelf->buf + pelf->ehdr->e_shoff);
  pelf->shnum = pelf->ehdr->e_shnum;
  // scan sections backwards, looking for symbol table
  for(Elf64_Shdr* p = &pelf->shdr[pelf->ehdr->e_shnum-1];
      p > pelf->shdr; p--){
    if(p->sh_type == SHT_SYMTAB){
      pelf->sh_symtab = p;
      break;
    }
  }
  pelf->psym = (Elf64_Sym*)(pelf->buf + pelf->sh_symtab->sh_offset);
  pelf->symnum = pelf->sh_symtab->sh_size / pelf->sh_symtab->sh_entsize;
  // string table associated with symbols
  pelf->str_sym = pelf->buf + pelf->shdr[pelf->sh_symtab->sh_link].sh_offset;
  return len;
}

  
// process elf symbols
void elf_syms(sElf* pelf){
  Elf64_Sym* psym = pelf->psym+2;
  for(U32 i=2;i<pelf->symnum;i++,psym++){
    U32 shi = psym->st_shndx; // get section we are referring to
    if(shi){
      if(shi < 0xFF00){
	psym->st_value += pelf->shdr[shi].sh_addr;
     //sym_dump(pelf,psym);
      }
    } else {
      
    }
    sym_dump(pelf,psym);

  }
}

void elf_process_symbols(sElf* pelf,pfElfSymProc proc){
  Elf64_Sym* psym = pelf->psym + pelf->symnum-1;
  for(U32 i=2;i<pelf->symnum;i++,psym--){
    (*proc)(psym);
  }
}
void process_rel(sElf* pelf, Elf64_Rela* prel, Elf64_Shdr* shto){
  U64 base = shto->sh_addr; // base address of image being fixed-up
  U64 p = base + prel->r_offset;
  Elf64_Sym* psym = &pelf->psym[ELF64_R_SYM(prel->r_info)];
  U64 s = psym->st_value;
  U64 a = prel->r_addend;
  switch(ELF64_R_TYPE(prel->r_info)){
  case R_X86_64_PC32:  //data access
  case R_X86_64_PLT32: //calls
    U32 fixup = (U32)(s+a-p);
    *((U32*)p) = fixup;
printf("Fixup: P:%lx A:%ld S:%lx S+A-P: %08x\n",p,a,s,fixup);        
    break;
  case R_X86_64_64: //data, pointer
    *((U64*)p) = s + a;
printf("Fixup: P:%lx A:%ld S:%lx S+A: %016lx\n",p,a,s,s+a);
    break;
  default:
    printf("Unknown relocation type\n");
    rel_dump(pelf,prel);
    break;
  }
}


static void process_rel_section(sElf* pelf, Elf64_Shdr* shrel){
  U32 relnum = shrel->sh_size / shrel->sh_entsize;
  Elf64_Rela* prel = (Elf64_Rela*)(pelf->buf + shrel->sh_offset);
  // applying relocations to this section
  Elf64_Shdr* shto = &pelf->shdr[shrel->sh_info];
  printf("Applying relocations against %lX, %ld\n",shto->sh_addr,shto->sh_size);
  for(U32 i=0; i<relnum; i++,prel++){
    process_rel(pelf,prel,shto);
  }
}

void elf_rels(sElf* pelf){
  printf("Rel sections:\n");
  Elf64_Shdr* shdr = pelf->shdr;  
  for(U32 i=0; i< pelf->shnum; i++,shdr++){
    if(SHT_RELA == shdr->sh_type){
  sechead_dump(pelf,i);
      process_rel_section(pelf,shdr);
    }
  }
}  
