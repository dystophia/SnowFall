#ifndef WELL_HEADER
#define WELL_HEADER

#include "util.h"

struct well {
	int32_t *mixBuffer;
        int32_t *v;
        uint32_t index;
        uint32_t *iRm1;
        uint32_t *iRm2;
        int32_t *i1;
        int32_t *i2;
        int32_t *i3;
};

void initWell(struct well *ctx);
void initWellState(struct well *ctx, struct fieldInfo *info);
void destroyWell(struct well *ctx);
void fill(struct well *ctx, int32_t *target, int count);
void mix(struct well *ctx, unsigned char *mix, int chunks);
void writeState(struct well *ctx, int fd);
void readState(struct well *ctx, int fd);

#endif
