typedef struct sSystem {
  sUnit** units;
  U32 nUnits;
} sSystem;


void sys_dump();
  
void sys_init();
void sys_add(sUnit* pu);
sUnit* sys_find_hash(U32 hash,U32* pi);
U64 sys_symbol_address(char* name);
sUnit* sys_load_elf(char* path);
void sys_load_two(char* path1, char* path2);
void sys_load_two1(char* path1, char* path2);
