// symtab.
#include <stdio.h>
#include <stdlib.h> //malloc
#include <string.h> //memcpy
#include <elf.h>
#include <dlfcn.h>

#include "global.h"
#include "elf.h"
#include "elfdump.h"
#include "hexdump.h"
#include "seg.h"
#include "unit.h"

extern sSeg scode;
extern sSeg sdata;

void usym_dump(sUnit*pu, U32 i){
  printf("%08x ",pu->dats[i].off);
  printf("%08x ",pu->hashes[i]);//,pu->dats[i].ostr);
  printf("%s\n",pu->strings + pu->dats[i].ostr);
  
}

void unit_dump(sUnit* pu){
  printf("-----\nUnit %s\n",pu->strings+1);
  printf("code: %p, %04d  ",scode.base + pu->oCode,pu->szCode);
  printf("data: %p, %04d  ",sdata.base + pu->oData,pu->szData);
  printf("%d symbols, %d globals\n",pu->nSyms,pu->nGlobs);
  
  for(U32 i=1;i<pu->nSyms;i++){
    usym_dump(pu,i);
  }  
  
}


#define FNV_PRIME 16777619
#define FNV_OFFSET_BASIS 2166136261

U32 string_hash(char*p){
  U32 hash = FNV_OFFSET_BASIS;
  U8 c;
  while((c=*p++)){
    hash = (U32)((hash ^ c) * FNV_PRIME);
  }
  return hash;
 }

/*
Ingest sections into code or data segment, assigning base addresses

Of interest are section that need allocation (SHF_ALLOC)..
Executable (SHF_EXECINSTR) sections get placed into the code seg; the
rest go into the data seg.  
* For now, we merge all data - BSS, vars, strings, ro or not
* We section honor alignment
*/ 
void unit_sections_from_elf(sUnit*pu,sElf* pelf){
  // unit segment positions
  pu->oCode = scode.fill;
  pu->oData = sdata.fill;
 
  //printf("Of interest:\n");
  Elf64_Shdr* shdr = pelf->shdr;  
  for(U32 i=0; i< pelf->shnum; i++,shdr++){
    U64 flags = shdr->sh_flags;
    if(flags & SHF_ALLOC){
      // select code or data segment to append
      sSeg*pseg = (flags & SHF_EXECINSTR) ? &scode : &sdata;
      // src and size of append
      U8* src = (shdr->sh_type == SHT_NOBITS)?
	0 : pelf->buf + shdr->sh_offset;
      seg_align(pseg,shdr->sh_addralign);
      shdr->sh_addr = (U64)seg_append(pseg,src,shdr->sh_size);
      //      sechead_dump(pelf,i);
    }
  }
  // update unit sizes
  pu->szCode = scode.fill - pu->oCode;
  pu->szData = sdata.fill - pu->oData;
}

// note: 0th symbol is not ingested.
void unit_symbols_from_elf(sUnit*pu,sElf* pelf){

  char* buf_str = (char*)malloc(0x10000);
  U32*  buf_hashes = (U32*)malloc(0x1000 * 4);
  sSym* buf_syms = (sSym*)malloc(0x1000 * sizeof(sSym));

  // 0th entry: unit name
  char* name = pelf->str_sym+1;
  char* pstr = buf_str;
  *pstr++ = 0;
  strcpy(pstr, name);
  pstr += (1 + strlen(name));
  buf_hashes[0]=string_hash(name);
  buf_syms[0].value=0;

  // reserve 0th
  U32 cnt = 1;
  void proc(Elf64_Sym* psym){
    // only defined global symbols make it to our table
    if(psym->st_shndx && ELF64_ST_BIND(psym->st_info)){
      char* name = pelf->str_sym + psym->st_name;
      U64 namelen = strlen(name)+1;
      strcpy(pstr,name);
      buf_hashes[cnt] = string_hash(name);
      buf_syms[cnt].ostr = pstr - buf_str; // string offset
      buf_syms[cnt].off = psym->st_value; // symbol addr
      
      pstr += namelen;
      cnt++;
    }
  }
  elf_process_symbols(pelf,proc);

  pu->nSyms = cnt;
  pu->nGlobs = cnt-1;
  pu->strings = (char*) realloc(buf_str,pstr-buf_str);
  pu->hashes  = (U32*)  realloc(buf_hashes,cnt*4);
  pu->dats =    (sSym*) realloc(buf_syms,cnt*sizeof(sSym));
}
/* 

void unit_symbols1(sUnit* pu,sElf* pelf){
  //  sSyms* psyms = (sSyms*) malloc(sizeof(psyms));
  // we ignore ELF sym[0], and sym[1] is filename
  U32 cnt = pelf->symnum-2;
  pu->nSyms = cnt;     
  
  U64 strings_size = pelf->sh_symtab->sh_size;
  pu->strings = (char*)malloc(strings_size);
  memcpy(pu->strings,pelf->str_sym,strings_size);

  pu->hashes = (U32*) malloc(cnt * 4);
  pu->dats   = (sSym*)malloc(cnt * sizeof(sSym));

  char* strbase  = pu->strings;
  pu->hash = string_hash(strbase+1);  // set unit hash

  Elf64_Sym* psym = pelf->psym + cnt+1;  // Starting from last symbol
  sSym* pdat   = pu->dats;
  U32*  phash  = pu->hashes;
  //  U64 dbase = (U64)sdata.base + pu->oData; // absolute data
  //U64 cbase = (U64)scode.base + pu->oCode;
  U32 globs = 0;
  // walk ELF symbol table backwards; 
  for(U32 i=0;i<cnt;i++,psym--,pdat++,phash++){
    printf("--");
  sym_dump(pelf,psym);
    *phash = string_hash(strbase + psym->st_name);
    pdat->ostr = (U32)psym->st_name;
    //    pdat->size = psym->st_size;
    if(ELF64_ST_BIND(psym->st_info)>0)
      globs++;
    //    pdat->type = ELF64_ST_TYPE(psym->st_info);
    // Convert ELF symbol addresses to unit offsets
    switch(ELF64_ST_TYPE(psym->st_info)){
    case STT_FUNC:
      pdat->seg = 1;
      pdat->off = (U32)(psym->st_value);
      break;
    case STT_OBJECT:
    case STT_NOTYPE:
    case STT_SECTION:
      pdat->seg = 0;
      pdat->off = (U32)(psym->st_value);
      break;
    default:
      break;
    }
    //    printf("%d %06x %04d %s\n", pdat->seg,pdat->off,pdat->size,strbase+psym->st_name);
  }
  pu->nGlobs = globs;
}
*/

U32 unit_find_hash(sUnit*pu,U32 hash){
  U32* p = pu->hashes+1;
  for(U32 i = 1;i<pu->nSyms; i++){
    if(hash==*p++)
      return i;
  }
  return 0;
}

/* units_find_hash

ppu points at a zero-terminated list of pointers to sUnits.

returns: 0 on fail or
 * low: index+1 of matching symbol; high: index of sUnit* in list
 * parameter ppu points at sUnit* containing it

 */
U64 units_find_hash(sUnit**ppu,U32 hash){
  sUnit* pu;
  U32 found;
  U64 i=0;
  while((pu=*ppu++)){
    found = unit_find_hash(pu,hash);
    if(found)
      return (i<<32)|found;
    i++;
  }
  return 0;
}


/*
U32 unit_sym_addr(sElf*pelf,sUnit*pu,U32 si){
  return 
}
*/

//typedef int (*pfun)(const char* s);

/* fake a library from a list of funs and names */
void unit_lib(sUnit*pu,char*name,U32 num,void**funs,char**names){
  static U8 buf[12]={0x48,0xB8,   // mov rax,?
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //64-bit address
    0xFF,0xE0}; // jmp rax

  pu->oCode = scode.fill;
  pu->oData = sdata.fill;
  // first pass: create per-function jump, compute strings size
  U32 namelen = strlen(name)+1;
  U32 strbytes=1+namelen;
  for(U32 i=0; i<num; i++){
    U8* p = seg_append(&scode,buf,sizeof(buf)); // compile jump
    *((void**)(p+2)) = funs[i]; // fixup to function address
    strbytes += (strlen(names[i])+1); // compute string total;
  }
  

  // allocate, accounting for extra 0th sym and hash
  pu->nGlobs = num;  //actual usable global symbols
  num++;
  pu->nSyms = num;   //0th is used for lib name
  pu->strings = (char*)malloc(strbytes);
  pu->hashes =   (U32*)malloc((num) * 4);
  pu->dats =    (sSym*)malloc((num) * sizeof(sSym));

  // second pass: create symbols
  char* pstr = pu->strings;  // string fill pointer
  *pstr++ = 0;               // start with 0
  strcpy(pstr,name);         // unit name, at 1
  pstr+=namelen;
  sSym* p = pu->dats;
  p->value = 0; //0th entry
  p++;
  U32* phash = pu->hashes;
  *phash++ = string_hash(name);

  for(U32 i=0; i<num-1; i++,p++){
    //    printf("processing %s\n",names[i]);
    *phash++ = string_hash(names[i]);
    strcpy(pstr,names[i]);
    p->seg=1;
    p->type=0;
    p->ostr = pstr - pu->strings;
    //  p->size = 12;i
    p->off = (U32)(U64)scode.base + pu->oCode + 12 * i;
    //
    pstr += (strlen(names[i])+1);
  } 
  // update bounds of unit
  pu->szCode = scode.fill - pu->oCode;
  pu->szData = sdata.fill - pu->oData;


  //  pfun pf = (pfun)p;
  // (*pf)("fuck you \n\n");
}

void unit_lib1(sUnit*pu,void* dlhan, U32 num,char*namebuf){
  static U8 buf[12]={0x48,0xB8,   // mov rax,?
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //64-bit address
    0xFF,0xE0}; // jmp rax

  pu->oCode = scode.fill;
  pu->oData = sdata.fill;

  pu->nGlobs = num - 1;
  pu->nSyms = num;
  pu->strings = namebuf; // using the one passed to us
  pu->hashes  = (U32*)malloc((num) * 4);
  pu->dats    = (sSym*)malloc((num) * sizeof(sSym));
  
  // first pass: create per-function jump, compute strings size

  U32* phash = pu->hashes;
  char* pstr = pu->strings+1; //start at name
  sSym* psym = pu->dats;

  *phash++ = string_hash(pstr);
  pstr += (strlen(pstr)+1);
  psym->value = 0;
  psym++;
  for(U32 i=1; i<num; i++){
    U8* addr = seg_append(&scode,buf,sizeof(buf)); // compile jump
    void* pfun = dlsym(dlhan,pstr);
    *((void**)(addr+2)) = pfun; // fixup to function address
    psym->off = (U32)(U64)addr;
    psym->seg  = 1;
    psym->type = 0;
    psym->ostr = pstr - pu->strings; // get string offset
    psym++;
    *phash++ = string_hash(pstr);
    pstr += (strlen(pstr)+1);        // find next string
  }
  
  // update bounds of unit
  pu->szCode = scode.fill - pu->oCode;
  pu->szData = sdata.fill - pu->oData;

}
