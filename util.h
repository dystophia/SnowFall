#ifndef UTIL_HEADER
#define UTIL_HEADER

#define min(a,b) (((a)<(b))?(a):(b))

struct fieldInfo {
        int field;
        int testnet;
        char *name;
        char *root;
	char *prefix;
	char *unit;
	uint64_t bytes;
	uint32_t gbytes;
	int threads;
};

int writep(int fd, unsigned char *buf, int n);
int pwritep(int fd, unsigned char *buf, int n, uint64_t position);
int readp(int fd, unsigned char *buf, int n);
void phex(void* mem, int size);
int openFile(char *directory, struct fieldInfo *info, char *suffix, int read);
void getFieldInfo(struct fieldInfo *target, int field, int testnet);

#endif
