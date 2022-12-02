// elvs 

typedef struct sElvs {
  sElf** pelfs; // * array of elf objects 
  sUnit** ppu;  // * array of units
  U32*  hashes; // * global hashlist
  U8*   ielf;   // * matching array of indices into pelvs
  U16*  isym;   // * index of symbol in its elf
  U32   nsyms;  // total number of symbols here
  U32   nelfs;    // number of elf objects

} sElvs;


void elvs_init(sElvs* pm, U32 cnt,char**paths);
void elvs_delete(sElvs* pm);
void elvs_dump(sElvs* pm);
U64 elvs_find(sElvs*pm, char* name);
U64 elvs_resolve_symbols(sElvs*pm);
U64 elvs_resolve_undefs(sElvs*pm);
void elvs_step2(sElvs*pm);
