#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "merkle.h"
#include "skein.h"
#include "util.h"

#define DECK_ENTRIES            1024
#define SNOW_MERKLE_HASH_LEN    16
#define MERKLE_CHUNK            (SNOW_MERKLE_HASH_LEN*1024*1024)

void *merkleThread(void *ptr) {
        struct merkleWork *work = (struct merkleWork*)ptr;

	Skein_256_Ctxt_t x;

        for(int steps=0; steps<10; steps++) {
                for(int i=0; i<work->hashes; i++) {
                        Skein_256_Init(&x, 128);
                        Skein_256_Update(&x, &work->buffer[SNOW_MERKLE_HASH_LEN * 2 * i], SNOW_MERKLE_HASH_LEN * 2);
                        Skein_256_Final(&x, &work->buffer[SNOW_MERKLE_HASH_LEN * i]);
                }
                work->hashes >>= 1;

                if(work->hashes == 1)
                        return NULL;
        }

        return NULL;
}

void snowMerkle(char *directory, struct fieldInfo *info) {
	char deck[] = "deck.a";
	int nthreads = 64;

	unsigned char *readBuffers = (unsigned char*)malloc(MERKLE_CHUNK * nthreads);
	pthread_t *threads = (pthread_t*)malloc(nthreads * sizeof(pthread_t));
	uint8_t *thread_states = (uint8_t*)malloc(nthreads);
	struct merkleWork *work = (struct merkleWork*)malloc(nthreads * sizeof(struct merkleWork));

	if(!readBuffers | !threads | !thread_states | !work) {
		printf("Error allocating merkle buffer\n");
		exit(-1);
	}

	int fd = openFile(directory, info, "snow");
	uint64_t size = info->bytes;


	int deckfd;
	for(;;) {
		deckfd = 0;

		if(size > 1024 * SNOW_MERKLE_HASH_LEN)
			deckfd = openFile(directory, info, deck);

		uint64_t position = 0;
		int tn = 0, tc = 0;

		for(int i=0; i<nthreads; i++) {
			thread_states[i] = 0;
			work[i].buffer = &readBuffers[MERKLE_CHUNK * i];
		}

		do {
			tn = (tn + 1) % nthreads;

			if(thread_states[tn]) {
				pthread_join(threads[tn], NULL);
				if(work[tn].hashes == 1) {
					printf("Calculated merkle root: ");
					phex(work[tn].buffer, 16);

					if(info->root)
						printf("Stored merkle root:     %s\n", info->root);
					else
						printf("No merkle root stored for field %i\n", info->field);

					close(deckfd);
					free(readBuffers);
					free(threads);
					free(thread_states);
					free(work);
					return;
				} else {
					writep(deckfd, work[tn].buffer, work[tn].hashes * SNOW_MERKLE_HASH_LEN);
				}
				thread_states[tn] = 0;
			}

			if(position >= size)
				continue;

			position += readp(fd, work[tn].buffer, min(MERKLE_CHUNK, size));

			work[tn].hashes = min(MERKLE_CHUNK, size) / SNOW_MERKLE_HASH_LEN;
			pthread_create(&threads[tn], NULL, merkleThread, (void*)&work[tn]);
			thread_states[tn] = 1;
			tc = 0;
		} while(tc++ < nthreads);

		close(fd);
		size >>= 10;

		deck[5]++;
		fd = deckfd;
		lseek(fd, 0, SEEK_SET);
	}
}

