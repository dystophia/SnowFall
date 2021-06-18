#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "skein.h"
#include "well.h"
#include "util.h"
#include "merkle.h"

#define MULTIPLICITY		128
#define PAGESIZE		(4*1024)
#define SNOWMONSTER_PAGESIZE	(PAGESIZE/4)
#define SNOWMONSTER_COUNT	(268435456 / SNOWMONSTER_PAGESIZE)
#define WRITE_CHUNK		(1024 * 1024)


#define PASSES                  7


struct snowMonster {
	int32_t *buffer; // 256MB, divided in 262144 chunks
	int position;
	int filled;
};

struct node {
	uint64_t val;
	struct node *left, *right;
};

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

void help() {
	printf("--- SnowFall for SnowBlossom cryptocurrency ---\n");
	printf("Usage: ./SnowFall [-f snowfield] [-d directory] [-h]\n");
	exit(0);
}

int main(int argc, char **argv) {
	int field = 0;
	int testnet = 0;
	char *directory = NULL;
	
	if(argc == 1)
		help();

	char mode = 0;
	for(int i=1; i<argc; i++) {
		if(argv[i][0] == '-')	
			mode = argv[i][1];
		else {
			if(mode == 'f')
				field = atoi(argv[i]);
			if(mode == 'd') 
				directory = argv[i];
		}
	}

	struct fieldInfo info;
	getFieldInfo(&info, field, testnet);

	if(info.name) {
		printf("Starting SnowFall for field %i (%s), %u GiB\n", field, info.name, info.gbytes);
	} else {
		printf("Starting SnowFall for field %i, %u GiB\n", field, info.gbytes);
	}

	uint64_t size = info.bytes;
	uint64_t mb_count = size / WRITE_CHUNK;
	uint64_t page_count = size / (uint64_t)PAGESIZE;
        uint64_t writes = (page_count * (uint64_t)PASSES) / MULTIPLICITY;
	
	int fd = openFile(directory, &info, "snow");

	struct well w;
	init(&w, &info);
	
	// Prepare Snow Monster
	struct snowMonster monster;

	monster.position = 0;
	monster.filled = 0;
	monster.buffer = (int32_t*)malloc(SNOWMONSTER_COUNT * SNOWMONSTER_PAGESIZE);
	if(!monster.buffer) {
		printf("Failed to allocate snowmonster memory\n");
		exit(-1);
	}

	// Initial write
	{
		unsigned char *writeBuffer = (unsigned char*)malloc(WRITE_CHUNK);
		if(!writeBuffer) {
			printf("Failed to allocate writebuffer memory\n");
			exit(-1);
		}
		
		fillMonster(&monster, &w);
		
		printf("Initial write ..\n");

		for(uint64_t i=0; i<mb_count; i++) {
			// Fill it with 1MB of data
			fill(&w, (int32_t*)writeBuffer, (WRITE_CHUNK / 4));

			writep(fd, writeBuffer, WRITE_CHUNK);

			mix(&w, (unsigned char*)getMonster(&monster), 256);
		
			fillMonster(&monster, &w);

			if(i % 1024 == 0) {
				printf("%lu GiB written\r", i / 1024);
				fflush(stdout);
			}
		}

		printf("Initial write completed. %lu GiB written.\n", mb_count / 1024);
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
			if(run % 1024 == 0) {
				float percent = (100 * run) / (float)writes;
				printf("Pass %lu of %lu (%.1f%%)\r", run, writes, percent);
				fflush(stdout);
			}

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

	close(fd);

	printf("Snowfield creation finished    \n");

	snowMerkle(directory, &info);	
}

