/* Load a single self-contained ELF object file and link */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <elf.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "global.h"

static char* str_SHT[]= {
  "NULL    ",            /* Section header table entry unused */
  "PROGBITS",/* Program data */
  "SYMTAB  ",	/* Symbol data */
  "STRTAB  ",		/* String table */
  "RELA    ",	  		/* Relocation entries with addends */
  "HASH    "	  ,		/* Symbol hash table */
  "DYNAMIC "	  ,		/* Dynamic linking information */
  "NOTE    "	  ,		/* Notes */
  "NOBITS  "	  ,		/* Program space with no data (bss) */
  "REL     "	,		/* Relocation entries, no addends */
  "SHLIB   "	  ,		/* Reserved */
  "DYNSYM  "	  ,		/* Dynamic linker symbol table */
  "INIT_ARRAY   "     ,	/* Array of constructors */
  "FINI_ARRAY   ",		/* Array of destructors */
  "PREINIT_ARRAY",		/* Array of pre-constructors */
  "GROUP        ",		/* Section group */
  "SYMTAB_SHNDX "		/* Extended section indices */
};
#include <ctype.h>
void* hd_line(void* ptr){
  U8*p=(U8*)ptr;
  printf("%p ",p);
  for(int i=0;i<16;i++){
    printf("%02X ",p[i]);
  }
  for(int i=0;i<16;i++){
    printf("%c",isprint(p[i])?p[i]:128);
  }
  printf("\n");
  return(p+16);
}

void* hd(void*ptr,int lines){
  for(int i=0;i<lines;i++){
    ptr = hd_line(ptr);
  }
  return ptr;
}

extern U8* buf;
extern Elf64_Ehdr* ehdr;
extern Elf64_Shdr* shdr  ;
extern Elf64_Shdr* sh_shstrtab;
extern Elf64_Shdr* sh_symtab;

char* get_str(Elf64_Shdr* sh_str,int str_idx){
  if(str_idx){
    if(str_idx > sh_str->sh_size-1){
      fprintf(stderr,"sym_str: string index too high\n");
      exit(1);
    } 
    return (char*)(buf + sh_str->sh_offset + str_idx);
  }else{
    return "-0-";
  }
}

char* section_name(U32 idx){
  if(idx<SHN_LORESERVE){
    if(!idx) return "-UNDEF-";
    return get_str(sh_shstrtab,shdr[idx].sh_name);
  } else {
    switch(idx){
    case SHN_ABS: return "-ABS-";
    case SHN_COMMON: return "-COMON-";
    case SHN_XINDEX: return "-XINDEX-";
    default: return "-UNK-";
    }
  }
}

char* sym_info_bind_str(U32 n){
  switch(ELF64_ST_BIND(n)){
  case 0: return "LOCAL";
  case 1: return "GLOBL";
  case 2: return "WEAK ";
  case 10: return "GNU_UNIQUE";
  default: return "BAD_BIND!";
  }
}

char* sym_info_type_str(U32 n){
  switch(ELF64_ST_TYPE(n)){
  case 0: return "NOTYPE";
  case 1: return "OBJECT";
  case 2: return "FUNC  ";
  case 3: return "SECTN ";
  case 4: return "FILE  ";
  case 5: return "COMMON";
  case 6: return "TLS   ";
  case 10: return "GNU_IFUNC";
  default: return "BAD_STYPE!";
  }
}
char* sym_other_vis_str(U32 n){
  switch(ELF64_ST_VISIBILITY(n)){
  case 0: return "vDEFAULT ";
  case 1: return "vINTERNAL";
  case 2: return "vHIDDEN  ";
  case 3: return "vPROTECT ";
  }
  return 0;
}
  
/* -------------------------------------------------------------*/
void sym_dump(Elf64_Shdr* sh_str,Elf64_Sym* psym){
  char* name = get_str(sh_str,psym->st_name);
  printf("%08lx %04ld ",psym->st_value,psym->st_size);
  printf("%s %s %s ",
	 sym_info_type_str(psym->st_info),
	 sym_info_bind_str(psym->st_info),
	 sym_other_vis_str(psym->st_other));
       
  printf("%s %d,in section:%s\n",name,(psym->st_shndx),section_name(psym->st_shndx));
  
}

void sechead_dump(Elf64_Shdr* psh){
    int sht = psh->sh_type;
    printf("%08lx %04lx ",psh->sh_addr,psh->sh_size);
    if(sht<SHT_NUM)
      printf("%s ",str_SHT[sht]);
    else
      printf("shtype %X ",sht);
    printf("%08lX ",psh->sh_flags);
    printf("\"%s\" ",get_str(sh_shstrtab,psh->sh_name));
    printf("\toff: %ld  size: %ld ",psh->sh_offset,psh->sh_size);
    printf("align: %ld",psh->sh_addralign);
    printf("\n");
}
void secheads_dump(){
  // sections
  for(int i=0;i<ehdr->e_shnum;i++){
    printf("%02d: ",i);
    sechead_dump(&shdr[i]);
  }
}

void symtab_dump(){
  printf("Symbol table at %ld, size %ld\n",sh_symtab->sh_offset,sh_symtab->sh_size);
  int sym_num = (int)sh_symtab->sh_size / sh_symtab->sh_entsize; //sizeof(Elf64_Sym);
  printf("entries: %d\n",sym_num);
  printf("Corresponding string table section %d\n",sh_symtab->sh_link);

  Elf64_Shdr* sh_str = &shdr[sh_symtab->sh_link];
  Elf64_Sym* syms = (Elf64_Sym*)(buf + sh_symtab->sh_offset);
  for(int i=0;i<sym_num;i++){
    printf("%d: ",i);
    sym_dump(sh_str,&syms[i]);
  }
}
char* reltype_str(U32 i){
  switch(i){
  case 0: return "NONE";
  case 1: return "64_64";
  case 2: return "PC32";
  case 3: return "GOT32";
  case 4: return "PLT32";
  default:
    fprintf(stderr,"reltype_str: implement rel type %d\n",i);
    exit(1);
  }
  return 0;
}
void rel_dump(Elf64_Rela* p){
  printf("%08lx +%02ld sym:%08lX, type:%s\n",
	 p->r_offset,
	 p->r_addend,
	 ELF64_R_SYM(p->r_info),
	 reltype_str((U32)ELF64_R_TYPE(p->r_info))
	 );
}
void reltab_dump(U32 rtab_isec){
  Elf64_Shdr* sh_reltab = &shdr[rtab_isec];
  Elf64_Rela*  reltab = (Elf64_Rela*) (buf + sh_reltab->sh_offset);
  U32 cnt = sh_reltab->sh_size / sh_reltab->sh_entsize;
  U32 i_sym_sh = sh_reltab->sh_link;
  U32 i_bin_sh = sh_reltab->sh_info;

  printf("Assoc. symtab idx %d; apply to section %d\n",i_sym_sh,i_bin_sh);
  for(int i=0;i<cnt;i++){
    rel_dump(&reltab[i]);
  }
}
void elf_dump(){
  //  printf("%d sections\n",ehdr->e_shnum);
  secheads_dump();
  symtab_dump();

}

