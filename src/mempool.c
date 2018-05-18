#include <stdlib.h>
#include <stdio.h>
#include "mempool.h"

/*
 * freelist constructor. Uses the 'mspace' parameter as a
 * pointer to a record in the memory pool.
 */
struct freelist *freelist_init(char *mspace) {
    struct freelist *n = malloc(sizeof(struct freelist));
    n->mem = mspace;
    n->next = NULL;
    return n;
}

/*
 * freelist destructor. Does NOT free any memory pointed
 * from it.
 */
struct freelist *freelist_del(struct freelist *fl) {
    fl->mem = NULL;
    struct freelist *n = fl->next;
    free(fl);
    return n;
}

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
    mp->memspace = malloc(cap * itemsize);
    if ( mp->memspace == NULL ) {
        return NULL;
    }
    mp->free = freelist_init(mp->memspace);

    struct freelist *iter = mp->free;
    for ( size_t i = 1; i < cap; ++i ) {
        iter->next = freelist_init(mp->memspace + (i * itemsize));
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
    struct freelist *iter = mp->free;
    while ( iter != NULL ) {
        iter = freelist_del(iter);
    }
    free(mp->memspace);
    free(mp);
}

/*
 * Takes itemsize memory from the memory pool.
 */
void *mempool_take(struct mempool *mp) {
    if ( mp->free != NULL ) {
        struct freelist *fl = mp->free;
        void *res = (void *) fl->mem;
        mp->free = freelist_del(fl);
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
    struct freelist *n = freelist_init((char *) mem);
    if ( n != NULL ) {
        n->next = mp->free;
        mp->free = n;
        mp->freesize++;
        return 1;
    }
    return 0;
}

/*
 * Enlarges the memory pool by adding another 'addition' items to the pool.
 */
int mempool_grow(struct mempool *mp, size_t addition) {
    size_t cap = mp->capacity;
    size_t itemsize = mp->itemsize;
    size_t totalsize = (addition + cap) * itemsize;

    char *resized = realloc(mp->memspace, totalsize);
    if ( resized ) {
        mp->memspace = resized;
        for ( size_t i = cap; i < (cap + addition); ++i ) {
            if ( !mempool_recycle(mp, mp->memspace + (i * itemsize)) ) {
                return 0;
            }
        }
        mp->capacity = (cap + addition);
        return 1;
    } else {
        return 0;
    }
}

