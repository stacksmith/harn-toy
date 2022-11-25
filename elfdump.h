
void elf_dump();
void reltab_dump(U32 i);
void rel_dump(Elf64_Rela* p);
void sym_dump(Elf64_Shdr* sh_str,Elf64_Sym* psym);
void sechead_dump(Elf64_Shdr* psh);
void secheads_dump();

char* get_str(Elf64_Shdr* sh_str,int idx);

U8* hd(U8*p,U32 l);
