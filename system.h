typedef struct sSystem {
  sUnit** units;
  U32 nUnits;
} sSystem;



  
void sys_init();
void sys_add(sUnit* pu);
sUnit* sys_find_hash(U32 hash,U32* pi);
U64 sys_symbol_address(char* name);
