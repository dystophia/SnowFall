#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "monster.h"

void initMonster(struct snowMonster *ctx) {
        ctx->position = 0;
        ctx->filled = 0;
        ctx->buffer = (int32_t*)malloc(SNOWMONSTER_COUNT * SNOWMONSTER_PAGESIZE);
        if(!ctx->buffer) {
                printf("Failed to allocate snowmonster memory\n");
                exit(-1);
        }
}

void destroyMonster(struct snowMonster *ctx) {
	free(ctx->buffer);
}

void fillMonster(struct snowMonster *ctx, struct well *w) {
        while(ctx->filled < SNOWMONSTER_COUNT) {
                fill(w, &ctx->buffer[((ctx->position + ctx->filled) % SNOWMONSTER_COUNT) * 256], 256);
                ctx->filled++;
        }
}

int32_t *getMonster(struct snowMonster *ctx) {
	int32_t *res = &(ctx->buffer[ctx->position * 256]);
        ctx->position = (ctx->position + 1) % SNOWMONSTER_COUNT;
        ctx->filled--;
        return res;
}

