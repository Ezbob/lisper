#include <stdlib.h>
#include <stdio.h>
#include "mempool.h"

/*
 * Constructor for the mempool.
 * The 'itemsize' size parameter is the size in bytes of the entities
 * that the mempool represents a pool of; and the 'cap' parameter is
 * the maximum number of such items that the pool is enable to supply
 * any consumers with.
 */
struct mempool *mempool_init(size_t itemsize, size_t cap) {
    struct mempool *mp = malloc(sizeof(struct mempool));
    mp->itemsize = itemsize;
    mp->capacity = cap;
    mp->memspace = malloc(cap * (itemsize + sizeof(struct freelist)));
    if ( mp->memspace == NULL ) {
        return NULL;
    }
    size_t blocksize = itemsize + sizeof(struct freelist);

    mp->free = (struct freelist *) mp->memspace;

    struct freelist *iter = mp->free;
    for ( size_t i = 1; i < cap; ++i ) {
        iter->next = (void *) (mp->memspace + i * blocksize);
        iter = iter->next;
    }
    iter->next = NULL;
    mp->freesize = cap;

    return mp;
}

/*
 * Memory pool destructor.
 * Deallocates the remaining records in the free lists, and deallocates
 * the heap memory allocate to represent the memory pool.
 */
void mempool_del(struct mempool *mp) {
    free(mp->memspace);
    free(mp);
}

/*
 * Takes itemsize memory from the memory pool.
 */
void *mempool_take(struct mempool *mp) {
    if ( mp->free != NULL ) {
        struct freelist *fl = mp->free;
        void *res = (void *) (fl + 1); // actual memory is next to free struct
        mp->free = fl->next;
        mp->freesize--;
        return res;
    } else {
        return NULL;
    }
}

/*
 * Puts itemsize size memory back into the pool,
 * by putting it on the freelist.
 */
int mempool_recycle(struct mempool *mp, void *mem) {
    struct freelist *memspace = mem;
    struct freelist *header = (memspace - 1);
        /* because we have casted memspace to freelist we offset by one freelist pointer */
    struct freelist *next = mp->free;
    header->next = next;
    mp->free = header;
    mp->freesize++;
    return 1;
}

