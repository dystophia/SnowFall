#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/types.h>

#include "merkle.h"
#include "skein.h"
#include "util.h"

void *merkleThread(void *ptr) {
        struct merkleWork *work = (struct merkleWork*)ptr;

	Skein_256_Ctxt_t x;

        for(int steps=0; steps<10; steps++) {
                work->hashes >>= 1;
                for(int i=0; i<work->hashes; i++) {
                        Skein_256_Init(&x, 128);
                        Skein_256_Update(&x, &work->buffer[SNOW_MERKLE_HASH_LEN * 2 * i], SNOW_MERKLE_HASH_LEN * 2);
                        Skein_256_Final(&x, &work->buffer[SNOW_MERKLE_HASH_LEN * i]);
                }

                if(work->hashes == 1)
                        return NULL;
        }

        return NULL;
}

void snowMerkle(char *directory, struct fieldInfo *info) {
	char deck[] = "deck.a";
	int nthreads = 16;

	unsigned char *readBuffers = (unsigned char*)malloc(MERKLE_CHUNK * nthreads);
	struct merkleWork *work = (struct merkleWork*)malloc(nthreads * sizeof(struct merkleWork));

	if(!readBuffers | !work) {
		printf("Error allocating merkle buffer\n");
		exit(-1);
	}

	int fd = openFile(directory, info, "snow", 0);
	uint64_t size = info->bytes;

	int deckfd;
	for(;;) {
		deckfd = 0;

		if(size > 1024 * SNOW_MERKLE_HASH_LEN)
			deckfd = openFile(directory, info, deck, 0);

		uint64_t position = 0;
		int tn = 0, tc = 0;

		for(int i=0; i<nthreads; i++) {
			work[i].state = 0;
			work[i].buffer = &readBuffers[MERKLE_CHUNK * i];
		}

		do {
			tn = (tn + 1) % nthreads;
			if(work[tn].state) {
				pthread_join(work[tn].thread, NULL);
				if(work[tn].hashes == 1) {
					printf("Calculated merkle root: ");
					phex(work[tn].buffer, 16);

					if(info->root)
						printf("Stored merkle root:     %s\n", info->root);
					else
						printf("No merkle root stored for field %i\n", info->field);

					close(deckfd);
					free(readBuffers);
					free(work);
					return;
				} else {
					writep(deckfd, work[tn].buffer, work[tn].hashes * SNOW_MERKLE_HASH_LEN);
				}
				work[tn].state = 0;
			}

			if(position >= size)
				continue;

			position += readp(fd, work[tn].buffer, min(MERKLE_CHUNK, size));
			work[tn].hashes = min(MERKLE_CHUNK, size) / SNOW_MERKLE_HASH_LEN;
			pthread_create(&work[tn].thread, NULL, merkleThread, (void*)&work[tn]);
			work[tn].state = 1;
			tc = 0;
		} while(tc++ < nthreads);

		close(fd);
		size >>= 10;

		deck[5]++;
		fd = deckfd;
		lseek(fd, 0, SEEK_SET);
	}
}

