TOP = ..

CC=gcc

CFLAGS=-Wall -O2 -std=c99 -fno-strict-aliasing -Wno-pointer-sign -Wno-sign-compare -Wno-unused-result -Wno-format-truncation -Wno-stringop-truncation 


test10: test10.c
	$(CC) -c $@ $^ $(CFLAGS) -I$(TOPSRC) 

elf1: elf1.c
	$(CC) -o $@ $^ $(CFLAGS) -I$(TOPSRC) -lm -ldl

elf2: elf2.c elfdump.c
	$(CC) -o $@ $^ $(CFLAGS) 

elf3: elf3.c elfdump.c seg.c
	$(CC) -o $@ $^ $(CFLAGS) 

clean:
	rm elf3 elf3.o elfdump.o *~
