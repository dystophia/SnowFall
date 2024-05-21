#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
	printf("Usage: ./SnowFall [-f snowfield] [-d directory] [-t|-s] [-n|-c] [-h]\n");
	printf("\n");
	printf("-f snowfield   Generate specific snowfield (default 0)\n");
	printf("-d directory   Set target directory (default current directory)\n");
	printf("-r blockdevice Use raw block device for snow file. Data on the device is overwritten, use with caution!\n");
	printf("-t             Generate teapot (Testnet) snowfield\n");
	printf("-s             Generate spoon (Regtest) snowfield\n");
	printf("-n             Generate from scratch, no bootstrap used\n");
	printf("-c             Create bootstrap file\n");
	printf("-h             Print this help\n");
	exit(0);
}

int main(int argc, char **argv) {
	int field = 0;
	int testnet = 0;
	char *directory = ".";
	char *raw = NULL;
	struct fieldInfo info;
	info.threads = 32;

	if(argc == 1)
		help();

	char mode = 0;
	int fromScratch = 0;

	for(int i=1; i<argc; i++) {
		if(argv[i][0] == '-')	
			mode = argv[i][1];
		else {
			if(mode == 'f')
				field = atoi(argv[i]);
			if(mode == 'd') 
				directory = argv[i];
			if(mode == 'r') 
				raw = argv[i];
			if(mode == 'p')
				info.threads = atoi(argv[i]);				
		}

		if(mode == 's') // Spoon
			testnet = 1;

		if(mode == 't')	// Teapot
			testnet = 2;

		if(mode == 'h')
			help();

		if(mode == 'n') // From scratch
			fromScratch = 1;

		if(mode == 'c') // Create bootstrap file
			fromScratch = 2;
	}

	// Cut trailing /
	if(directory[strlen(directory)-1] == '/')
		directory[strlen(directory)-1] = 0;

	getFieldInfo(&info, field, testnet);

	// Maximum of 32 threads, snowmonster limits us here
	info.threads = min(32, info.threads);
	
	int bootfd = openFile(directory, &info, "boot", 1, NULL);
	if(bootfd > 0 && !fromScratch) {
		snowFallBoot(directory, &info, bootfd, raw);
		snowMerkle(directory, &info, raw);
	} else if(fromScratch < 2) {
		snowFall(directory, &info, raw);
		snowMerkle(directory, &info, raw);
	} else {
		createBoot(directory, &info);
	}
	
	if(raw && fromScratch != 2) {
		printf("Snowfield created on block device %s.\nUse command\n\n  dd if=%s of=%s.%i.snow bs=1048576 count=%lu\n\nto copy the field.\n", raw, raw, info.prefix, info.field, info.bytes / 1048576);
	}
}

