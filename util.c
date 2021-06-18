#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "util.h"

int writep(int fd, unsigned char *buf, int n) {
        int w = 0;
        do {
                int bytes = write(fd, &buf[w], n-w);
                if(bytes < 1) {
                        printf("Error writing\n");
                        exit(-1);
                }
                w += bytes;
        } while(w < n);
	return n;
}

int readp(int fd, unsigned char *buf, int n) {
        int r = 0;
        do {
                int bytes = read(fd, &buf[r], n-r);
                if(bytes < 1) {
                        printf("Error reading\n");
                        exit(-1);
                }
                r += bytes;
        } while(r < n);
	return n;
}

int openFile(char *directory, struct fieldInfo *info, char *suffix) {
	char *fileName = (char*)malloc(strlen(directory) + strlen(suffix) + 34);

	// Create directory if needed:
        sprintf(fileName, "%s/%s.%i", directory, info->prefix, info->field);
	mkdir(fileName, 0755);

        sprintf(fileName, "%s/%s.%i/%s.%i.%s", directory, info->prefix, info->field, info->prefix, info->field, suffix);
	printf("Creating file %s\n", fileName);

        int fd = open(fileName, O_RDWR | O_CREAT | __O_LARGEFILE, 0644);

	if(!fd) {
		printf("Error opening file %s\n", fileName);
		free(fileName);
		exit(-1);
	}

	free(fileName);

	return fd;
}

void phex(void* mem, int size) {
    int i;
    unsigned char* p = (unsigned char*)mem;
    for (i = 0; i < size; i++) {
        printf("%02x", p[i]);
    }
    printf("\n");
}

void getFieldInfo(struct fieldInfo *target, int field, int testnet) {
	if(field < 0 || field > 32) {
		printf("Field out of range\n");
		exit(-1);
	}

	int fieldsDefined = 12;
	char * fieldNames[] = {
		"cricket",
		"stoat",
		"ocelot",
		"pudu",
		"badger",
		"capybara",
		"llama",
		"bugbear",
		"hippo",
		"shai-hulud",
		"avanc"
	};

	char * fieldRoots[] = {
		"2c363d33550f5c4da16279d1fa89f7a9",
		"4626ba6c78e0d777d35ae2044f8882fa",
		"2948b321266dfdec11fbdc45f46cf959",
		"33a2fb16f08347c2dc4cbcecb4f97aeb",
		"0cdcb8629bef77fbe1f9b740ec3897c9",
		"e9abff32aa7f74795be2aa539a079489",
		"e68678fadb1750feedfa11522270497f",
		"147d379701f621ebe53f5a511bd6c380",
		"533ae42a04a609c4b45464b1aa9e6924",
		"ecdb28f1912e2266a71e71de601117d2",
		"cc883468a08f48b592a342a2cdf5bcba",
		"a2a4076f6cde947935db06e5fc5bbd14"
	};

	target->field = field;
	target->testnet = testnet;
	target->name = (field < fieldsDefined) ? fieldNames[field] : NULL;
	target->root = (field < fieldsDefined) ? fieldRoots[field] : NULL;
        target->bytes = 1ULL << (30 + field);
	target->gbytes = 1 << field;
	target->prefix = "snowblossom";
}


