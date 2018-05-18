#ifndef LISPER_MEMPOOL
#define LISPER_MEMPOOL

#include <stdlib.h>

struct freelist {
    char *mem;
    struct freelist *next;
} freelist; 

struct mempool {
    size_t itemsize;
    size_t capacity;
    size_t freesize;
    char *memspace;
    struct freelist *free;
};

struct mempool *mempool_init(size_t itemsize, size_t capacity);

void mempool_del(struct mempool *mp);
void *mempool_take(struct mempool *mp);
int mempool_recycle(struct mempool *mp, void *mem);
int mempool_grow(struct mempool *mp, size_t addition);

#endif

