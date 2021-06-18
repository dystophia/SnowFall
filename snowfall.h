#ifndef SNOWFALL_HEADER
#define SNOWFALL_HEADER

struct node {
        uint64_t val;
        struct node *left, *right;
};

void snowFall(unsigned char *directory, struct fieldInfo *info, int bootstrap);

#endif
