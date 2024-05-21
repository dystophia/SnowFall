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

int pwritep(int fd, unsigned char *buf, int n, uint64_t position) {
        int w = 0;
        do {
                int bytes = pwrite(fd, &buf[w], n-w, position + w);
                if(bytes < 1) {
                        printf("Error writing                  \n");
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
                        printf("Error reading                  \n");
                        exit(-1);
                }
                r += bytes;
        } while(r < n);
	return n;
}

int openFile(char *directory, struct fieldInfo *info, char *suffix, int read, char *raw) {
	char *fileName = (char*)malloc(strlen(directory) + strlen(suffix) + 34);

	// Create directory if needed:
	int fd;

	if(raw) {
		fd = open(raw, O_RDWR | __O_LARGEFILE);
		if(!fd) {
			printf("Error opening raw device %s\n", raw);
			exit(-1);
		}
	} else if(read) {
        	sprintf(fileName, "bootstrap/%s.%i.%s", info->prefix, info->field, suffix);
	        fd = open(fileName, O_RDONLY | __O_LARGEFILE);
	} else {
        	sprintf(fileName, "%s/%s.%i", directory, info->prefix, info->field);
		mkdir(fileName, 0755);

        	sprintf(fileName, "%s/%s.%i/%s.%i.%s", directory, info->prefix, info->field, info->prefix, info->field, suffix);
		printf("Creating file %s\n", fileName);
	        fd = open(fileName, O_RDWR | O_CREAT | __O_LARGEFILE, 0644);
		if(!fd) {
			printf("Error opening file %s\n", fileName);
			free(fileName);
			exit(-1);
		}
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
		"shrew",
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

	int fieldsDefinedSpoon = 15;
	char * fieldRootsSpoon[] = {
		"da4eb1edd0969e39f86a139b7b43ba0e",
		"6a52fd075f6f581a9e6fb7c0a30ec8fe",
		"4f1a1233f12f08cdf15b73e49cc1636f",
		"4497d77025a5edf059ad5ab8f069a015",
		"ee9753a4d6f4dcb64faca5113f66a75d",
		"4c8395c86c195a997af873ec34ba5366",
		"64b276dd37f50790acf749eeeed3d56e",
		"08bfebc48f74292e9714238533c7859b",
		"c9ed0de2305a244e7e43d2757b1202c2",
		"46297d54bdd557c4098143e177edaaca",
		"656670bb7ff68a3c769e58a213c283e4",
		"105452019473852df4f2e5d31041d1c3",
		"8afc4a35b48641444fbf67c4750bdbdc",
		"e43a9b072cc6c1ae4bdfc59bce8b2e3e",
		"72005cfeb75f0498c360c7dc5fb6263a"
	};

	int fieldsDefinedTeapot = 15;
	char * fieldRootsTeapot[] = {
		"bacc447f89ee2b623721083ed9437842",
		"27aa4228bf39c6b6a5164ea3e050bfcc",
		"47873faca24808fbb334af74ffa3ce08",
		"97f8303394d267dfe4bf65243bd3740e",
		"3587f96c4e6651dd900e462d3404c03d",
		"ca1cedce13472a2e030c5c91933879a7",
		"0b3297f9b20e38ecfd66bc881a19412f",
		"72ec45325df2b918aaba7891491e0ad0",
		"6b1b0b675d6ecd3f6a07a924eb920754",
		"25cee1c0e6b5e6c7bbeee0756e0ca6cd",
		"b30e10f6c72b72f244cfddf5932f65f4",
		"5734ca926b818f8b6d39ff18e6b6eaa1",
		"3723f5d367352542e2ad83ab9c95e133",
		"a19edec3d956ddbf55e49f2665bbb55f",
		"8900685be6cc3721cf4cbdc8033e2ce3"
	};


	target->field = field;
	target->testnet = testnet;
	target->name = (field < fieldsDefined) ? fieldNames[field] : NULL;
	target->gbytes = 1 << field;

	if(testnet == 1) {		// spoon (Regtest)
		target->unit = "MiB";
		target->bytes = 1ULL << (20 + field);
		target->prefix = "spoon";
		target->root = (field < fieldsDefinedSpoon) ? fieldRootsSpoon[field] : NULL;

	} else if(testnet == 2) {	// teapot (Testnet)
		target->unit = "MiB";
		target->bytes = 1ULL << (20 + field);
		target->prefix = "teapot";
		target->root = (field < fieldsDefinedTeapot) ? fieldRootsTeapot[field] : NULL;
	} else {			// Mainnet
		target->unit = "GiB";
		target->bytes = 1ULL << (30 + field);
		target->prefix = "snowblossom";
		target->root = (field < fieldsDefined) ? fieldRoots[field] : NULL;
	}
}


