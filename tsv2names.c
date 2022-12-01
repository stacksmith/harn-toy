/* Load a single elf object file and fixup */
#include <stdio.h>
#include <stdlib.h> //exit
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <dlfcn.h>
#include "global.h"
#include "hexdump.h"
char* lbuf;
U32 nlines;
U16 oline[512];
void* dlhan;

U64 l_load(char* path){
  int fd = open(path,O_RDONLY);
  if(fd<0) {
    printf("Error opening %s\n",path);
    return 0;
  }
  off_t len = lseek(fd, 0, SEEK_END);
  if(fd<0){
    printf("Error seeking end\n");
    return 0;
  }
  lbuf = (char*)mmap(0, len, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
  if(!lbuf) {
    printf("Error mapping'ing %ld bytes\n",len);
    return 0;
  }
  close(fd);
  return (U32)len;
}


U32 lines_parse(){
  oline[0]=0;
  U32 i = 1;
  char*p = lbuf;
  while((p = strchr(p,10))){
    *p++=0;
    oline[i++] = (p - lbuf);
    //    printf("%d, %d\n",i,(U32)(p - lbuf));
  }  
  return i-1;
}
void lines_adjust(){
  for(U32 i=0;i<nlines;i++){
    char* p = lbuf+oline[i];
    char* p1 = strchr(p,9);
    *p1=0;
  }
}

void lines_dump(){
  for(U32 i=0;i<nlines;i++){
    printf("%s ",(lbuf + oline[i]));
  }
}

U32 syms_lookup(FILE*f){
  U32 cnt = 0;
  for(U32 i=0;i<nlines;i++){
    char* name = lbuf + oline[i];
    void* p = dlsym(dlhan,name);
    if(p){
      fputs(name,f);
      fputs("\n",f);
      cnt++;
    } else {
      fprintf(stderr," %s\n",name);
    }
  }
  return cnt;
}

typedef int (*pf1)(const char*);

int main(int argc,char** argv){
  U64 size = l_load(argv[1]);
  printf("loaded libdoc %ld\n",size);
  nlines = lines_parse();
  printf("Parsed %d lines!\n",nlines);
  lines_adjust();
  //  lines_dump();

  dlhan = dlopen("/usr/lib/x86_64-linux-gnu/libc.so.6",RTLD_NOW);
    dlhan = dlopen("libc.so.6",RTLD_NOW);

  FILE* f = fopen(argv[2],"w");
  if(!f) {
    printf("Could not open %s\n",argv[2]);
    exit(1);
  }
  U32 cnt = syms_lookup(f);
  fclose(f);
  printf("%d names are usable\n",cnt);

}
