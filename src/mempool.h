#ifndef LISPER_MEMPOOL
#define LISPER_MEMPOOL

#include <stdlib.h>

struct mempool {
    char *memspace;
    freelist_t free;
    size_t itemsize;
    size_t capacity;
    size_t takencount;
    struct mempool *next;
};

struct mempool *mempool_init(size_t itemsize, size_t capacity);
void mempool_del(struct mempool *mp);
int mempool_hasaddr(struct mempool *mp, void *mem);
void *mempool_take(struct mempool *mp);
int mempool_recycle(struct mempool *mp, void *mem);

#endif

