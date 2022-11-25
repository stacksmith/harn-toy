/* ==============================================================

A fillable area for code or data

  ==============================================================*/
typedef struct sSeg {
  U8* base; // ptr of allocation/mapping
  U8* fillptr;
  U8* end;
  char name[8];
} sSeg;

void seg_dump(sSeg* pseg);
int  seg_alloc(sSeg* pseg,char*name,U64 req_size, void* req_addr, U32 prot);
U8*  seg_append(sSeg* pseg,U8* start,U64 size);
