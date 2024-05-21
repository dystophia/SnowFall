#ifndef SNOWFALL_HEADER
#define SNOWFALL_HEADER

#include "well.h"
#include "monster.h"

struct node {
	uint64_t val;
	struct node *left, *right;
};

struct workAssignment {
	pthread_t thread;
	struct well *w;
	struct snowMonster *monster;
	unsigned char *writeBuffer;
	int fd;
	uint64_t offset;	
	int state;
};

void showInformation(struct fieldInfo *info);
void snowFall(char *directory, struct fieldInfo *info, char *raw);
void snowFallBoot(char *directory, struct fieldInfo *info, int bootfd, char *raw);
void createBoot(char *directory, struct fieldInfo *info);
void *initialStep(void *ptr);

#endif
