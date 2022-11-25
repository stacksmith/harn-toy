TOP = ..

CC=gcc

CFLAGS=-Wall -O2 -std=c99 -fno-strict-aliasing -Wno-pointer-sign -Wno-sign-compare -Wno-unused-result -Wno-format-truncation -Wno-stringop-truncation 


harn: harn.c elfdump.c seg.c hexdump.c
	$(CC) -o $@ $^ $(CFLAGS) 

clean:
	rm elf3 elf3.o elfdump.o *~
