#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "skein.h"
#include "well.h"
#include "util.h"
#include "merkle.h"
#include "monster.h"

#define MULTIPLICITY		128
#define PAGESIZE		(4*1024)
#define WRITE_CHUNK		(1024 * 1024)

#define PASSES                  7

struct node {
	uint64_t val;
	struct node *left, *right;
};

void help() {
	printf("--- SnowFall for SnowBlossom cryptocurrency ---\n");
	printf("Usage: ./SnowFall [-f snowfield] [-d directory] [-t|-s] [-h]\n");
	printf("\n");
	printf("-f snowfield   Generate specific snowfield (default 0)\n");
	printf("-d directory   Set target directory (default current directory)\n");
	printf("-t             Generate teapot (Testnet) snowfield\n");
	printf("-s             Generate spoon (Regtest) snowfield\n");
	printf("-h             Print this help\n");
	exit(0);
}

int main(int argc, char **argv) {
	int field = 0;
	int testnet = 0;
	char *directory = ".";
	
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

		if(mode == 's') // Spoon
			testnet = 1;

		if(mode == 't')	// Teapot
			testnet = 2;
	}

	// Cut trailing /
	if(directory[strlen(directory)-1] == '/')
		directory[strlen(directory)-1] = 0;
	


	struct fieldInfo info;
	getFieldInfo(&info, field, testnet);

	if(info.name) {
		printf("Starting SnowFall for field %i (%s), %u %s\n", field, info.name, info.gbytes, info.unit);
	} else {
		printf("Starting SnowFall for field %i, %u %s\n", field, info.gbytes, info.unit);
	}

	uint64_t size = info.bytes;
	uint64_t mb_count = size / WRITE_CHUNK;
	uint64_t page_count = size / (uint64_t)PAGESIZE;
        uint64_t writes = (page_count * (uint64_t)PASSES) / MULTIPLICITY;
	
	int fd = openFile(directory, &info, "snow");

	struct well w;
	initWell(&w, &info);
	
	// Prepare Snow Monster
	struct snowMonster monster;
	initMonster(&monster);

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
				nodes[m].val = __builtin_bswap64(nodes[m].val) % page_count;
				nodes[m].left = NULL;
				nodes[m].right = NULL;
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

				//readahead(fd, nodes[m].val * PAGESIZE, PAGESIZE);
			}
			
			for(int m=0; m<MULTIPLICITY; m++) {
				int read = 0;
				do {
					int bytes = pread(fd, &existing[m * PAGESIZE + read], PAGESIZE - read, nodes[m].val * PAGESIZE + read);
					if(bytes < 1) {
						printf("Error reading from file\n");
						exit(-1);
					}
					read += bytes;
				} while(read < PAGESIZE);

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

	destroyMonster(&monster);
	destroyWell(&w);

	close(fd);

	printf("Snowfield creation finished    \n");

	snowMerkle(directory, &info);	
}

