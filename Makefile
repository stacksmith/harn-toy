TOP = ..

CC=gcc

CFLAGS=-Wall -O2 -std=c99 -fno-strict-aliasing -Wno-pointer-sign -Wno-sign-compare -Wno-unused-result -Wno-format-truncation -Wno-stringop-truncation 


harn: harn.c elf.c elfdump.c seg.c hexdump.c unit.c
	$(CC) -o $@ $^ $(CFLAGS) 

clean:
	rm harn harn.o *.o *~
