#ifndef UTIL_HEADER
#define UTIL_HEADER

#define min(a,b) (((a)<(b))?(a):(b))

struct fieldInfo {
        int field;
        int testnet;
        char *name;
        char *root;
	char *prefix;
	uint64_t bytes;
	uint32_t gbytes;
};

int writep(int fd, unsigned char *buf, int n);
int readp(int fd, unsigned char *buf, int n);
void phex(void* mem, int size);
int openFile(char *directory, struct fieldInfo *info, char *suffix);
void getFieldInfo(struct fieldInfo *target, int field, int testnet);



#endif
