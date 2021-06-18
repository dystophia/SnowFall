#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "well.h"
#include "util.h"
#include "monster.h"
#include "snowfall.h"

#define MULTIPLICITY		128
#define PAGESIZE		(4*1024)
#define WRITE_CHUNK		(1024 * 1024)
#define PASSES                  7

void snowFall(unsigned char *directory, struct fieldInfo *info, int bootstrap) {
	if(info->name) {
		printf("Starting SnowFall for field %i (%s), %u %s\n", info->field, info->name, info->gbytes, info->unit);
	} else {
		printf("Starting SnowFall for field %i, %u %s\n", info->field, info->gbytes, info->unit);
	}

	uint64_t mb_count = info->bytes / WRITE_CHUNK;
	uint64_t page_count = info->bytes / (uint64_t)PAGESIZE;
        uint64_t writes = (page_count * (uint64_t)PASSES) / MULTIPLICITY;
	
	int fd = openFile(directory, info, "snow");
	
	if(bootstrap)
		bootstrap = openFile(directory, info, "boot");

	// Prepare well
	struct well w;
	initWellState(&w, info);
	
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

				if(bootstrap)
					writeState(&w, bootstrap);
			}
		}

		printf("Initial write completed. %lu GiB written.\n", mb_count / 1024);
		free(writeBuffer);
	}
	if(bootstrap)
		close(bootstrap);

	//return;
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

				readahead(fd, nodes[m].val * PAGESIZE, PAGESIZE);
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
}

