#include <stdint.h>
#include <stdlib.h>

#define INTS	1391
#define CHARS	(4*INTS)

struct well {
	int32_t *v;
	uint32_t index;
	uint32_t *iRm1;
	uint32_t *iRm2;
	int32_t *i1;
	int32_t *i2;
	int32_t *i3;
};

void setSeed(struct well *ctx, int32_t *seed, int seedLength) {
	for(int i=0; i<seedLength; i++)
		ctx->v[i] = seed[i];

        for(int i=seedLength; i<INTS; ++i) {
		uint64_t l = ctx->v[i - seedLength];
                ctx->v[i] = (uint32_t) ((1812433253L * (l ^ (l >> 30)) + i) & 0xffffffffL);
        }

        ctx->index = 0;
}

void init(struct well *ctx, int32_t *seed) {
	ctx->v = (int32_t*)malloc(CHARS);
        ctx->index  = 0;

        ctx->iRm1 = (uint32_t*)malloc(CHARS);
        ctx->iRm2 = (uint32_t*)malloc(CHARS);
        ctx->i1   = (int32_t*)malloc(CHARS);
        ctx->i2   = (int32_t*)malloc(CHARS);
        ctx->i3   = (int32_t*)malloc(CHARS);
        for (int j=0; j<INTS; ++j) {
            ctx->iRm1[j] = (j + 1390) % INTS;
            ctx->iRm2[j] = (j + 1389) % INTS;
            ctx->i1[j]   = (j + 23)   % INTS;
            ctx->i2[j]   = (j + 481)  % INTS;
            ctx->i3[j]   = (j + 229)  % INTS;
        }

        setSeed(ctx, seed, 32);
}

int32_t next(struct well *ctx) {
    	uint32_t indexRm1 = ctx->iRm1[ctx->index];
        uint32_t indexRm2 = ctx->iRm2[ctx->index];

        int32_t v0       = ctx->v[ctx->index];
        int32_t vM1      = ctx->v[ctx->i1[ctx->index]];
        int32_t vM2      = ctx->v[ctx->i2[ctx->index]];
        int32_t vM3      = ctx->v[ctx->i3[ctx->index]];

        int32_t z0       = (0xFFFF8000 & ctx->v[indexRm1]) ^ (0x00007FFF & ctx->v[indexRm2]);
        int32_t z1       = (v0 ^ (v0 << 24))  ^ (vM1 ^ (((uint32_t)vM1) >> 30));
        int32_t z2       = (vM2 ^ (vM2 << 10)) ^ (vM3 << 26);
        int32_t z3       = z1      ^ z2;
        int32_t z2Prime  = ((z2 << 9) ^ (((uint32_t)z2) >> 23)) & 0xfbffffff;
        int32_t z2Second = ((z2 & 0x00020000) != 0) ? (z2Prime ^ 0xb729fcec) : z2Prime;
        int32_t z4       = z0 ^ (z1 ^ (((uint32_t)z1) >> 20)) ^ z2Second ^ z3;

        ctx->v[ctx->index]= z3;
        ctx->v[indexRm1]  = z4;
        ctx->v[indexRm2] &= 0xFFFF8000;
        ctx->index        = indexRm1;

        z4 ^= (z4 <<  7) & 0x93dd1400;
        z4 ^= (z4 << 15) & 0xfa118000;

	return z4;
}

void fill(struct well *ctx, int32_t *target, int count) {
	for(int i=0; i<count; i++)
		target[i] = next(ctx);
}


