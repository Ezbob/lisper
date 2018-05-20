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
struct mempool *mempool_init(size_t itemsize, size_t poolsize) {
    struct mempool *mp = malloc(sizeof(struct mempool));
    if ( mp == NULL ) {
        return NULL;
    }
    size_t blocksize = itemsize + sizeof(char *);
    mp->capacity = poolsize * blocksize;
    mp->itemsize = itemsize;
    mp->memspace = malloc(mp->capacity);
    if ( mp->memspace == NULL ) {
        return NULL;
    }
    mp->next = NULL;
    mp->free = (char **) (mp->memspace);
    mp->takencount = 0;

    char **iter = mp->free;
    for ( size_t i = 1; i < poolsize; ++i ) {
        void *next = (mp->memspace + i * blocksize);
        *iter = next; // make current free pointer point to start of next block
        iter = (char **) next; // advance to next free pointer
    }
    *iter = NULL;

    return mp;
}

/*
 * Memory pool destructor.
 * Deallocates the remaining records in the free lists, and deallocates
 * the heap memory allocate to represent the memory pool.
 */
void mempool_del(struct mempool *mp) {
    struct mempool *iter = mp;
    while ( iter != NULL ) {
        struct mempool *next = iter->next;
        free(iter->memspace);
        free(iter);
        iter = next;
    }
}

/*
 * Takes itemsize memory from the memory pool.
 */
void *mempool_take(struct mempool *mp) {
    if ( mp->free != NULL ) {
        freelist_t head = mp->free;
        void *res = (void *) (head + 1); // actual memory is next to free pointer
        mp->takencount++;
        mp->free = (void *) *head;
        return res;
    }

    struct mempool *iter = mp;
    while ( iter->next != NULL && iter->free == NULL ) { // search chain of pools
        iter = iter->next;
    }

    if ( iter->next == NULL && iter->free == NULL ) { // every pool on the chain is full
        iter->next = mempool_init(mp->itemsize, mp->takencount);
        iter = iter->next;
    }

    freelist_t head = iter->free;
    void *res = (void *) (head + 1);
    iter->takencount++;
    iter->free = (void *) *head;
    return res;
}

/*
 * Check if memory is in pool
 */
int mempool_hasaddr(struct mempool *mp, void *mem) {
    return mp->memspace <= ((char *) mem) && (mp->memspace + mp->capacity) > ((char *) mem);
}

/*
 * Puts itemsize size memory back into the pool,
 * by putting it on the freelist.
 */
int mempool_recycle(struct mempool *mp, void *mem) {
    struct mempool *iter = mp;
    while ( iter != NULL && !mempool_hasaddr(iter, mem) ) {
        iter = iter->next;
    }
    if ( iter ) {
        freelist_t header = ( (freelist_t) mem ) - 1; // next free pointer is to the left of the data
        *header = (void *) iter->free;
        iter->takencount--;
        iter->free = header;
        return 0;
    }
    return 1;
}

