#ifndef MERKLE_HEADER
#define MERKLE_HEADER

#include "util.h"

struct merkleWork {
        unsigned char *buffer;
        int hashes;
};

void *merkleThread(void *ptr);
void snowMerkle(char *directory, struct fieldInfo *info);

#endif
