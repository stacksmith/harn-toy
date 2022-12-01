#include <stdio.h>
#include <stdlib.h> //exit
#include <string.h>
#include <dlfcn.h> 

#include "global.h"

#include "elf.h"
#include "unit.h"
/* lib.c

A library is a kind of a unit containing a set of bindings to a linux DSO.

A text file containing cr-terminated lines, each containing the 
name of a symbol that can be resolved in the library, is used to
create a proper ELF-like stringtable containing
0 libname 0 bindings... 0

*/
// load a text file containing cr-terminated names, and
// create a proper nametable
char* lib_load_names(char*path,char* name,U32*pcnt){
  FILE*f = fopen(path,"r");
  if(!f) {
    fprintf(stderr,"Unable to open %s\n",path);
    exit(1);
  }
  U64 namelen = strlen(name);
  U64 len;
  fseek(f,0,SEEK_END);
  len = ftell(f);
  fseek(f,0,SEEK_SET);

  char* buf = (char*)malloc(1+namelen+1+len);
  char*p = buf;
  *p++ = 0;
  strcpy(p,name);
  p+= namelen;
  *p++ = 0;
  U64 read = fread(p,1,len,f);
  if(read !=len){
    fprintf(stderr,"Failed reading %s got %ld of %ld\n",path,read,len);
    exit(1);
  }
  fclose(f);

  U32 i = 0;
  // now, replace crs with 0, and count them
  while((p = strchr(p,10))){
    *p++ = 0;
    i++;
  }
  *pcnt = i+1;
  
  return buf;	 
}

extern sUnit** srch_list;

U64 global_symbol_address(char* name){
  sUnit** ppu = srch_list;
  U32 i = units_find_hash(ppu,string_hash(name));
  if(!i){
    printf("Undefined symbol '%s'\n",name);
    exit(1);
  }
//      printf("Undefined %s found: %lx\n",name,i);
  sUnit* pu = *ppu;
  return pu->dats[i].off; // set elf sym value
}


sUnit* lib_make(char* dllpath,char*namespath,char*name){
  printf("Ingesting dll %s, names in %s, making %s\n",
	 dllpath,namespath,name);
  U32 symcnt;
  char* strings = lib_load_names(namespath,name,&symcnt);
  //  printf("loaded %d names\n",symcnt);
  void* dlhan = dlopen(dllpath,RTLD_NOW);
  if(!dlhan){
    fprintf(stderr,"Unable to open dll %s\n",dllpath);
    exit(1);
  }
  sUnit* pulib = (sUnit*)malloc(sizeof(sUnit));
  unit_lib(pulib,dlhan,symcnt,strings);
  return pulib;
}

