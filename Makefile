CC ?= gcc
CFLAGS ?= -O3 -std=gnu99
SnowFall:	SnowFall.c Makefile skein.o skein_block.o well.c
		$(CC) $(CFLAGS) -lpthread -c -o SnowFall SnowFall.c skein.o skein_block.o

skein.o:	skein.c skein.h skein_port.h brg_types.h brg_endian.h skein_iv.h
		$(CC) $(CFLAGS) -c -o skein.o skein.c

skein_block.o:	skein_block.c skein.h
		$(CC) $(CFLAGS) -c -o skein_block.o skein_block.c

clean:
		rm skein.o skein_block.o SnowFall
