#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "skein.h"
#include "well.c"

#define MULTIPLICITY		128
#define PAGESIZE		(4*1024)
#define SNOWMONSTER_PAGESIZE	(PAGESIZE/4)
#define SNOWMONSTER_COUNT	(268435456 / SNOWMONSTER_PAGESIZE)
#define PASSES			7
#define DECK_ENTRIES		1024
#define SNOW_MERKLE_HASH_LEN	16
#define MERKLE_CHUNK		(SNOW_MERKLE_HASH_LEN*1024*1024)


#define min(a,b) (((a)<(b))?(a):(b))

int32_t *mixBuffer;

struct snowMonster {
	int32_t *buffer; // 256MB, divided in 262144 chunks
	int position;
	int filled;
};

struct node {
	uint64_t val;
	struct node *left, *right;
};

void phex(void* mem, int size) {
    int i;
    unsigned char* p = (unsigned char*)mem;
    for (i = 0; i < size; i++) {
        printf("%02x", p[i]);
    }
    printf("\n");
}

void fillMonster(struct snowMonster *monster, struct well *w) {
    while(monster->filled < SNOWMONSTER_COUNT) {
	fill(w, &monster->buffer[((monster->position + monster->filled) % SNOWMONSTER_COUNT) * 256], 256);
	monster->filled++;
    }
}

int32_t *getMonster(struct snowMonster *monster) {
    int32_t *res = &(monster->buffer[monster->position * 256]);
    monster->position = (monster->position + 1) % SNOWMONSTER_COUNT;
    monster->filled--;
    return res;
}

void mix(struct well *w, unsigned char *mix, int chunks) {
	fill(w, mixBuffer, 1391 - chunks);

	int32_t *mixInt = (int32_t*)mix;

	for(int i=0; i<chunks; i++)
		w->v[i] = __builtin_bswap32(mixInt[i]);

	for(int i=chunks; i<1391; i++)
		w->v[i] = __builtin_bswap32(mixBuffer[i-chunks]);

	w->index = 0;
}

void *merkleThread(void *ptr) {
	unsigned char *buffer = (unsigned char*)ptr;
	int hashes = MERKLE_CHUNK / SNOW_MERKLE_HASH_LEN;
	Skein_256_Ctxt_t x;

	for(int steps=0; steps<10; steps++) {
		for(int i=0; i<hashes; i++) {
			Skein_256_Init(&x, 128);
			Skein_256_Update(&x, &buffer[SNOW_MERKLE_HASH_LEN * 2 * i], SNOW_MERKLE_HASH_LEN * 2);
			Skein_256_Final(&x, &buffer[SNOW_MERKLE_HASH_LEN * i]);
		}
		hashes /= 2;
	}

	return NULL;
}

void writep(int fd, unsigned char *buf, int n) {
	int written = 0;
	do {
		int bytes = write(fd, &buf[written], n-written);
		if(bytes < 1) {
			printf("Error writing\n");
			exit(-1);
		}
		written += bytes;
	} while(written < n);	
}

int main(int argc, char **argv) {
	mixBuffer = (int32_t*)malloc(4540);	// Enough for 1024 bytes mixing data

	if(argc != 2) {
		printf("Usage: ./SnowFall <field>\n");
		exit(-1);
	}

	int field = atoi(argv[1]);

	uint64_t size = 1073741824;
	size *= 1 << field;

	uint64_t mb_count = size / (1024*1024);
	uint64_t page_count = size / (uint64_t)PAGESIZE;
	uint64_t writes = (page_count * (uint64_t)PASSES) / MULTIPLICITY;
	
	char fieldSeed[100];
	int fieldSeedBytes = sprintf(fieldSeed, "snowblossom.%i", field);

	printf("Starting snowfall with seed %s for snowfield %i\n", fieldSeed, field);

	// Open target files
	fieldSeedBytes = sprintf(fieldSeed, "snowblossom.%i.snow", field);
	int fd = open(fieldSeed, O_RDWR | O_CREAT | __O_LARGEFILE, 0644);
			
	// Initial hash
	char target[128];
	Skein1024_Ctxt_t x;
	Skein1024_Init(&x, 1024);
	Skein1024_Update(&x, fieldSeed, fieldSeedBytes);
	Skein1024_Final(&x, target);

	// java endian convert:
	int32_t *t = (int32_t*)target;
	int32_t c[32];
	for(int i=0; i<32; i++) c[i] = __builtin_bswap32(t[i]);

	// Initialize well
	struct well w;
	init(&w, c);
	
	// Prepare Snow Monster
	struct snowMonster monster;
	monster.position = 0;
	monster.filled = 0;
	monster.buffer = (int32_t*)malloc(SNOWMONSTER_COUNT*SNOWMONSTER_PAGESIZE);
	if(!monster.buffer) {
		printf("Failed to allocate snowmonster memory\n");
		exit(-1);
	}

	// Initial write
	{
		unsigned char *writeBuffer = (unsigned char*)malloc(1024*1024);
		if(!writeBuffer) {
			printf("Failed to allocate writebuffer memory\n");
			exit(-1);
		}
		
		fillMonster(&monster, &w);
		
		printf("Initial write ..\n");

		for(uint64_t i=0; i<mb_count; i++) {
			// Fill it with 1MB of data
			fill(&w, (int32_t*)writeBuffer, 1024*256);

			int written = 0;
			do {
				int bytes = write(fd, &writeBuffer[written], 1024*1024-written);
				if(bytes < 1) {
					printf("Failed to write initial data\n");
					exit(-1);
				}
				written += bytes;
			} while(written < 1024*1024);

			mix(&w, (unsigned char*)getMonster(&monster), 256);
		
			fillMonster(&monster, &w);

			if(i % 1024 == 0)
				printf("%lu GB written\n", i / 1024);
		}

		printf("Initial write completed. %lu GB written.\n", mb_count / 1024);
		free(writeBuffer);
	}

	// Step 2
	{
		unsigned char *writeBuffer = (unsigned char*)malloc(MULTIPLICITY * PAGESIZE);
		unsigned char *existing = (unsigned char*)malloc(MULTIPLICITY * PAGESIZE);
		struct node *nodes = (struct node*)malloc(MULTIPLICITY * sizeof(struct node));

		if(!writeBuffer || !existing || !nodes) {
			printf("Failed to allocate memory\n");
			exit(-1);
		}

		for(uint64_t run=0; run < writes; run++) {
			if(run % 1024 == 0)
				printf("Pass %lu of %lu\n", run, writes);

			for(int m=0; m<MULTIPLICITY; m++) {
				mix(&w, (unsigned char*)getMonster(&monster), 256);
				fill(&w, (int32_t*)&(writeBuffer[m * PAGESIZE]), PAGESIZE / 4);
				fill(&w, (int32_t*)&(nodes[m]), 2);
			}
			fillMonster(&monster, &w);

			for(int m=0; m<MULTIPLICITY; m++) {
				uint64_t position = __builtin_bswap64(nodes[m].val) % page_count;
				nodes[m].val = position;
				nodes[m].left = NULL;
				nodes[m].right = NULL;
			}

			for(int m=0; m<MULTIPLICITY; m++) {
				if(m > 0) {
					struct node *it = &nodes[0];
					do {
						if(nodes[m].val == it->val) {
							nodes[m].val = (nodes[m].val + 1) % page_count;
							it = &nodes[0];
						} else if(nodes[m].val < it->val) {
							if(it->left == NULL) {
								it->left = &nodes[m];
								it = NULL;
							} else
								it = it->left;
						} else {
							if(it->right == NULL) {
								it->right = &nodes[m];
								it = NULL;
							} else
								it = it->right;
						}
					} while(it);
				}

				int position = nodes[m].val * PAGESIZE;
				int read = 0;
				do {
					int bytes = pread(fd, &existing[m * PAGESIZE + read], PAGESIZE - read, nodes[m].val * PAGESIZE + read);
					if(bytes < 1) {
						printf("Error reading from file\n");
						exit(-1);
					}
					read += bytes;
				} while(read < PAGESIZE);
			}
			
			for(int m=0; m<MULTIPLICITY; m++) {
				mix(&w, &existing[m * PAGESIZE], 1024);

				for(int i=0; i<PAGESIZE; i++)
					writeBuffer[m * PAGESIZE + i] ^= existing[m * PAGESIZE + i];

				int position = nodes[m].val * PAGESIZE;
				int write = 0;
				do {
					int bytes = pwrite(fd, &writeBuffer[m * PAGESIZE + write], PAGESIZE - write, nodes[m].val * PAGESIZE + write);
					if(bytes < 1) {
						printf("Error writing to file\n");
						exit(-1);
					}
					write += bytes;
				} while(write < PAGESIZE);

				mix(&w, &writeBuffer[m * PAGESIZE], 1024);
			}
		}
		free(writeBuffer);
		free(existing);
		free(nodes);
	}

	free(mixBuffer);

	// Deck generation
	{
		char deck = 'a';
		int nthreads = 64;

		unsigned char *readBuffers = (unsigned char*)malloc(MERKLE_CHUNK * nthreads);
		pthread_t *threads = (pthread_t*)malloc(nthreads * sizeof(pthread_t));
		uint8_t *thread_states = (uint8_t*)malloc(nthreads);

		if(!readBuffers | !threads | !thread_states) {
			printf("Error allocating merkle buffer\n");
			exit(-1);
		}

		for(;;) {
			sprintf(fieldSeed, "snowblossom.%i.deck.%c", field, deck);
			int deckfd = open(fieldSeed, O_RDWR | O_CREAT | __O_LARGEFILE, 0644);

			printf("Creating %s\n", fieldSeed);


			uint64_t position = 0;
			int tn = 0;
		
			for(int i=0; i<nthreads; i++)
				thread_states[i] = 0;

			do {
				if(thread_states[tn]) {
					pthread_join(threads[tn], NULL);
					writep(deckfd, &readBuffers[MERKLE_CHUNK * tn], min(MERKLE_CHUNK / 1024, size / 1024));
					thread_states[tn] = 0;
				}

				int read = 0;
				do {
					int bytes = pread(fd, &readBuffers[MERKLE_CHUNK * tn + read], MERKLE_CHUNK - read, position);
					if(bytes < 1) {
						printf("Error reading from file\n");
						exit(-1);
					}
					read += bytes;
					position += bytes;
				} while(read < min(MERKLE_CHUNK, size));
					
				pthread_create(&threads[tn], NULL, merkleThread, (void*)&readBuffers[MERKLE_CHUNK * tn]);
				thread_states[tn] = 1;

				tn = (tn + 1) % nthreads;
			} while(position < size);

			// Catch remaining threads:
			int tc = nthreads;
			while(tc--) {
				if(thread_states[tn]) {
					pthread_join(threads[tn], NULL);
					writep(deckfd, &readBuffers[MERKLE_CHUNK * tn], min(MERKLE_CHUNK / 1024, size / 1024));
					thread_states[tn] = 0;
				}
				tn = (tn + 1) % nthreads;
			}

			close(fd);

			size >>= 10;

			if(size < 1024*32) {
				close(deckfd);
				free(readBuffers);
				free(threads);
				free(thread_states);
				break;
			}

			deck++;
			fd = deckfd;
			lseek(fd, 0, SEEK_SET);
		}
	}
	printf("Snowfield creation finished\n");
}

