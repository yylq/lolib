
/*
 * fcache_hashtable.c
 *
 */

#include <assert.h>
#include <math.h>
#include "fast_hashtable.h"
#include "fast_memory.h"
#include "fast_math.h"
#include "fast_string.h"
#include "fast_lock.h"

#define FAST_HASH4(x) ((x) = ((x) << 5) + (x) + *key++) 

size_t
fast_hashtable_hash_hash4(const void *data, size_t data_size, size_t hashtable_size);

size_t
fast_hashtable_hash_hash4(const void *data, size_t data_size, size_t hashtable_size)
{

    size_t      ret = 0;
    size_t      loop = data_size >> 3;
    const char *key = data;

    if (data_size & (8 - 1)) {
        FAST_HASH4(ret);
    }

    while (loop--) {
        FAST_HASH4(ret);
        FAST_HASH4(ret);
        FAST_HASH4(ret);
        FAST_HASH4(ret);
        FAST_HASH4(ret);
        FAST_HASH4(ret);
        FAST_HASH4(ret);
        FAST_HASH4(ret);
    }
    return ret % hashtable_size;
}

size_t
fast_hashtable_hash_key8(const void *data,
    size_t data_size, size_t hashtable_size)
{
    int         bnum;
    size_t      i;
    size_t      ret;
    size_t      one;
    size_t      n = 0;
    size_t      loop = data_size >> 3;
    const char *s = data;
    
    if ((bnum = (data_size % 8))) {
        loop++;
    }
    for (i = 0; i < loop; i++) {
        one = *((uint64_t *)s);
        if (bnum && i == loop - 1) {
           one >>= (bnum * 8);
        }
        n ^= 271 * one; 
        s += 8;
    }
    ret = n ^ (loop * 271);
    
    return ret % hashtable_size;
}


size_t
fast_hashtable_hash_low(const void *data,
    size_t data_size, size_t hashtable_size)
{
    const char *s = data;
    size_t n = 0;
    size_t j = 0;
    while (*s && j < data_size) {
        j++;
        n = n*31 + string_xxctolower(*s);
        s++;
    }
    
    return n % hashtable_size;
}

/*
 *  hash_create - creates a new hash table, uses the cmp_func
 *  to compare keys.  Returns the identification for the hash table;
 *  otherwise returns NULL.
 */
fast_hashtable_t *
fast_hashtable_create(FAST_HASHTABLE_CMP *cmp_func, size_t hash_sz,
    FAST_HASHTABLE_HASH *hash_func, fast_mem_allocator_t *allocator)
{
    fast_hashtable_t     *ht;
    unsigned int         err_no;

    if (allocator) {
        ht = allocator->calloc(allocator, sizeof(fast_hashtable_t), &err_no);
        ht->allocator = allocator;
    } else {
        ht = memory_calloc(sizeof(fast_hashtable_t));
    }
    
    if (!ht) {
        return NULL;
    }
    
    if (fast_hashtable_init(ht, cmp_func, hash_sz, hash_func, 
        allocator) == FAST_HASHTABLE_OK) {
        return ht;
    }
    
    if (allocator) {
        allocator->free(allocator, ht, &err_no);
    } else {
        memory_free(ht, sizeof(fast_hashtable_t));
    }
    
    return NULL;
}

//init a hash table with cmp_func and other info
int
fast_hashtable_init(fast_hashtable_t *ht, FAST_HASHTABLE_CMP *cmp_func,
    size_t hash_sz, FAST_HASHTABLE_HASH *hash_func,
     fast_mem_allocator_t *allocator)
{    
    unsigned int     err_no;

    if (!ht) {
        return FAST_HASHTABLE_ERROR;
    }

    //set hash size
    ht->size = !hash_sz ? FAST_HASHTABLE_DEFAULT_SIZE :
        fast_math_find_prime(hash_sz);
        
    //calloc buckets, freebkts and lock if need
    if (allocator) {
        ht->allocator = allocator;
        ht->buckets = allocator->calloc(allocator,
            ht->size * sizeof(fast_hashtable_link_t *), &err_no);
    } else {
        ht->buckets = memory_calloc(ht->size *
            sizeof(fast_hashtable_link_t *));
        ht->allocator = NULL;
    }

    if (!ht->buckets) {
        return FAST_HASHTABLE_ERROR;
    }
       
    ht->cmp = cmp_func;
    ht->hash = hash_func;
    ht->count = 0;

    return FAST_HASHTABLE_OK;
}

/*
 *  hash_join - joins a hash_link under its key lnk->key
 *  into the hash table 'ht'.  
 *
 *  It does not copy any data into the hash table,
 *  only add links pointers to the hash table.
 */
int
fast_hashtable_join(fast_hashtable_t *ht, fast_hashtable_link_t *hl)
{
    size_t              i;
    
    if (!ht || !hl) {
        return FAST_HASHTABLE_ERROR;
    }
    i = ht->hash(hl->key, hl->len, ht->size);
    hl->next = ht->buckets[i];
    ht->buckets[i] = hl;
    ht->count++;
    
    return FAST_HASHTABLE_OK;
}

/*
 *  hash_lookup - locates the item under the key 'k' in the hash table
 *  'ht'.  Returns a pointer to the hash bucket on success; otherwise
 *  returns NULL.
 */
void *
fast_hashtable_lookup(fast_hashtable_t *ht, const void *key, size_t len)
{
    size_t                i = 0;
    fast_hashtable_link_t *walker = NULL;
    
    if (!key || !ht) {
        return NULL;
    }
    // Modify by Dingyj: add len info
    i = ht->hash(key, len, ht->size);
    for (walker = ht->buckets[i]; walker; walker = walker->next) {
        if (!walker->key) {
            return NULL;
        }
        if ((ht->cmp) (key, walker->key, len) == 0) {
            return (walker);
        }
        assert(walker != walker->next);
    }
    
    return NULL;
}

/*
 *  hash_remove_link - deletes the given hash_link node from the 
 *  hash table 'ht'.  Does not free the item, only removes it
 *  from the list.
 *
 *  An assertion is triggered if the hash_link is not found in the
 *  list.
 */
int
fast_hashtable_remove_link(fast_hashtable_t *ht, fast_hashtable_link_t *hl)
{
    int                      i;
    fast_hashtable_link_t   **link;

    if (!ht || !hl) {
        return FAST_HASHTABLE_ERROR;
    }
    
    i = ht->hash(hl->key, hl->len, ht->size);
    for (link = &ht->buckets[i]; *link; link = &(*link)->next) {
        if (*link == hl) {
            *link = hl->next;
            ht->count--;
            return FAST_HASHTABLE_OK;
        }
    }

    return FAST_HASHTABLE_ERROR;
}

/*
 *  hash_get_bucket - returns the head item of the bucket 
 *  in the hash table 'hid'. Otherwise, returns NULL on error.
 */
fast_hashtable_link_t *
fast_hashtable_get_bucket(fast_hashtable_t *ht, uint32_t bucket)
{
    if (bucket >= ht->size) {
        return NULL;
    }

    return ht->buckets[bucket];
}

void
fast_hashtable_free_memory(fast_hashtable_t *ht)
{
    unsigned int        err_no;

    if (!ht) {
        return;
    }
    
    if (ht->allocator) {
        if (ht->buckets) {
            ht->allocator->free(ht->allocator, ht->buckets, &err_no);
        }
       
        ht->allocator->free(ht->allocator, ht, &err_no);
    } else {
        if (ht->buckets) {
            memory_free(ht->buckets,
                ht->size * sizeof(fast_hashtable_link_t *));
        }
        memory_free(ht, sizeof(fast_hashtable_t));
    }
}

void
fast_hashtable_free_items(fast_hashtable_t *ht,
    void (*free_object_func)(void *), void *param)
{
	size_t                i;
	fast_hashtable_link_t *walker;
	fast_hashtable_link_t *next;
    
	if (!ht || !free_object_func) {
    	return;
    }
    for (i = 0; i < ht->size; i++) {
        for (walker = ht->buckets[i]; walker;) {
            next = walker->next;
            free_object_func(walker);
            walker = next;
            ht->count--;
        }
    }
}

int fast_hashtable_empty(fast_hashtable_t *ht)
{
    return ht && ht->count ? FAST_HASHTABLE_FALSE : FAST_HASHTABLE_TRUE;
}

