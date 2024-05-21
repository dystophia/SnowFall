#ifndef MERKLE_HEADER
#define MERKLE_HEADER

#include "util.h"

#define DECK_ENTRIES            1024
#define SNOW_MERKLE_HASH_LEN    16
#define MERKLE_CHUNK            (SNOW_MERKLE_HASH_LEN*1024*1024)

struct merkleWork {
	pthread_t thread;
	unsigned char *buffer;
        int hashes;
	int state;
};

void *merkleThread(void *ptr);
void snowMerkle(char *directory, struct fieldInfo *info, char *raw);

#endif
