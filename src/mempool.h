#ifndef LISPER_MEMPOOL
#define LISPER_MEMPOOL

#include <stdlib.h>

struct mempool {
    unsigned char *memspace; /* pointer to byte-sized array that contains all the blocks */
    unsigned char **free; /* free list pointer. Points either to the next free block
        or null if no more blocks are free*/
    size_t itemsize; /* byte size of the indiviual types contained in a block */
    size_t capacity; /* byte capacity of the mempool */
    size_t takencount; /* how many blocks has been taken */
    struct mempool *next; /* next mempool pointer allows for
        allocation of more mempools when the capacity has been reached */
};

struct mempool *mempool_init(size_t itemsize, size_t capacity);
void mempool_del(struct mempool *mp);
int mempool_hasaddr(struct mempool *mp, void *mem);
void *mempool_take(struct mempool *mp);
int mempool_recycle(struct mempool *mp, void *mem);

#endif

