// symtab.h

/* Symbols are maintained in 
 * A string-list with 0-terminated symbol names;
 * A hash-list with 32-bit hashes of each name;
 * A symdata-list, with matching data:
 ** type  obj,func,file                    4
 ** vis   local,global                     1
 ** pad
 ** str   offset to string table          16
 ** doff  data offset in current unit     32
 */

typedef struct sSym {
  union {
    U16 pad;
    struct {
      U32 type:4;
      U32 vis:1;
      U32 unk:11;
    };
  };
  U16 ostr;
  U32 off;
} sSym;


typedef struct sUnit {
  U32 oCode;       // code segment offset 
  U32 szCode;      // size of code
  U32 oData;       // data segment offset
  U32 szData;
  
  U32 nSyms;
  char*  strings;
  U32*   hashes;
  sSym*  dats;
} sUnit;

  

U32 fnv1a(char*p);
void unit_ingest(sUnit*pu,sElf* pelf);
