CC ?= gcc

SnowFall:	SnowFall.c Makefile skein.o skein_block.o well.c
		$(CC) -o SnowFall -O3 SnowFall.c skein.o skein_block.o -lpthread

skein.o:	skein.c skein.h skein_port.h brg_types.h brg_endian.h skein_iv.h
		$(CC) -c -O3 -o skein.o skein.c

skein_block.o:	skein_block.c skein.h
		$(CC) -c -O3 -o skein_block.o skein_block.c

clean:
		rm skein.o skein_block.o SnowFall
