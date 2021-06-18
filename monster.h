#ifndef MONSTER_HEADER
#define MONSTER_HEADER

#include "well.h"

#define SNOWMONSTER_PAGESIZE    1024
#define SNOWMONSTER_COUNT       (268435456 / SNOWMONSTER_PAGESIZE)

struct snowMonster {
        int32_t *buffer; // 256MB, divided in 262144 chunks
        int position;
     	int filled;
};

void initMonster(struct snowMonster *ctx);
void destroyMonster(struct snowMonster *ctx);
int32_t *getMonster(struct snowMonster *ctx);
void fillMonster(struct snowMonster *ctx, struct well *w);

#endif
