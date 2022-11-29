#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <elf.h>
#include "global.h"
#include "elf.h"


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

