#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "skein.h"
#include "well.h"
#include "util.h"
#include "merkle.h"
#include "monster.h"
#include "snowfall.h"

#define MULTIPLICITY		128
#define PAGESIZE		(4*1024)
#define WRITE_CHUNK		(1024 * 1024)

#define PASSES                  7

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

	snowFall(directory, &info, 1);

	snowMerkle(directory, &info);	
}

