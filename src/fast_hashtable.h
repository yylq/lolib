/*
 *  fcache_hashtable.h
 */

#ifndef _FAST_HASHTABLE_H
#define _FAST_HASHTABLE_H

#define  FAST_HASHTABLE_DEFAULT_SIZE       7951
#define  FAST_HASHTABLE_STORE_DEFAULT_SIZE 16777217

#include "fast_lock.h"
#include "fast_queue.h"
#include "fast_mem_allocator.h"

typedef void    FAST_HASHTABLE_FREE(void *);
typedef int     FAST_HASHTABLE_CMP(const void *, const void *, size_t);
typedef size_t  FAST_HASHTABLE_HASH(const void *, size_t, size_t);

enum {
    FAST_HASHTABLE_COLL_LIST,
    FAST_HASHTABLE_COLL_NEXT//store
};

#define FAST_HASHTABLE_ERROR     -1
#define FAST_HASHTABLE_OK        0

#define FAST_HASHTABLE_FALSE     0
#define FAST_HASHTABLE_TRUE      1

typedef struct fast_hashtable_link_s fast_hashtable_link_t;
struct fast_hashtable_link_s {
    void                    *key;
    size_t                   len;
    fast_hashtable_link_t    *next;
};

typedef struct fast_hashtable_errno_s {
    unsigned int        hash_errno;
    unsigned int        allocator_errno;
    fast_lock_errno_t    lock_errno;
} fast_hashtable_errno_t;

typedef struct fast_hashtable_s {
    fast_hashtable_link_t   **buckets;
    FAST_HASHTABLE_CMP       *cmp;         // compare function
    FAST_HASHTABLE_HASH      *hash;        // hash function
    fast_mem_allocator_t     *allocator;   // create on shmem
    size_t                   size;        // bucket number
    int                      coll;        // collection algrithm
    int                      count;       // total element that inserted to hashtable
} fast_hashtable_t;

//harmful modified
//#define hashtable_link_make(str) {(void*)(str), sizeof((str)) - 1, NULL}
#define fast_hashtable_link_make(str) {(void*)(str), sizeof((str)) - 1, NULL,NULL}

//harmful modified
//#define hashtable_link_null      {NULL, 0, NULL,NULL}
#define fast_hashtable_link_null      {NULL, 0, NULL,NULL}


FAST_HASHTABLE_HASH fast_hashtable_hash_hash4;
FAST_HASHTABLE_HASH fast_hashtable_hash_key8;
FAST_HASHTABLE_HASH fast_hashtable_hash_low;

fast_hashtable_t *
fast_hashtable_create(FAST_HASHTABLE_CMP *cmp_func, size_t hash_sz,
    FAST_HASHTABLE_HASH *hash_func, fast_mem_allocator_t *allocator);

int fast_hashtable_init(fast_hashtable_t *ht, FAST_HASHTABLE_CMP *cmp_func,
    size_t hash_sz, FAST_HASHTABLE_HASH *hash_func, fast_mem_allocator_t *);
int   fast_hashtable_empty(fast_hashtable_t *ht);
int   fast_hashtable_join(fast_hashtable_t *, fast_hashtable_link_t *);
int   fast_hashtable_remove_link(fast_hashtable_t *, fast_hashtable_link_t *);
void *fast_hashtable_lookup(fast_hashtable_t *, const void *, size_t len);
void  fast_hashtable_free_memory(fast_hashtable_t *);
void fast_hashtable_free_items(fast_hashtable_t *ht,
    void (*free_object_func)(void*), void*);
fast_hashtable_link_t *fast_hashtable_get_bucket(fast_hashtable_t *, uint32_t);


#endif

