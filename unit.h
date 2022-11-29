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
  // segment data
  U32 oCode;       // code segment offset 
  U32 szCode;      // size of code
  U32 oData;       // data segment offset
  U32 szData;
  // symbol data
  char*  strings;  // TODO: for now malloc'ed...
  U32*   hashes;   // TODO: for now malloc'ed...
  sSym*  dats;     // TODO: for now malloc'ed...
  U32    nSyms;
} sUnit;

  
void unit_dump(sUnit* pu);
U32 fnv1a(char*p);
void unit_sections(sUnit*pu,sElf* pelf);
void unit_symbols(sUnit*pu,sElf* pelf);
