CC ?= gcc
CFLAGS ?= -O3 -std=gnu99

SnowFall:	main.c well.o skein.o skein_block.o merkle.o util.o monster.o snowfall.o
		$(CC) $(CFLAGS) -o SnowFall main.c skein.o well.o skein_block.o util.o merkle.o monster.o snowfall.o -lpthread

snowfall.o:	snowfall.c snowfall.h
		$(CC) $(CFLAGS) -c -o snowfall.o snowfall.c

well.o:		well.c well.h
		$(CC) $(CFLAGS) -c -o well.o well.c

merkle.o:	merkle.c merkle.h
		$(CC) $(CFLAGS) -c -o merkle.o merkle.c

util.o:		util.c util.h
		$(CC) $(CFLAGS) -c -o util.o util.c

skein.o:	skein.c skein.h skein_port.h brg_types.h brg_endian.h skein_iv.h
		$(CC) $(CFLAGS) -c -o skein.o skein.c

monster.o:	monster.c monster.h
		$(CC) $(CFLAGS) -c -o monster.o monster.c

skein_block.o:	skein_block.c skein.h
		$(CC) $(CFLAGS) -c -o skein_block.o skein_block.c

clean:
		rm -f SnowFall skein_block.o skein.o well.o merkle.o util.o
