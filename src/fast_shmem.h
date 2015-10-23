//by harmful
#ifndef _FAST_SHMEM_H
#define _FAST_SHMEM_H

#include <sys/mman.h>
#include <sys/queue.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "fast_queue.h"
#include "fast_string.h"

#define FAST_SHMEM_DEFAULT_MAX_SIZE  ((size_t)10<<20) //10MB
#define FAST_SHMEM_DEFAULT_MIN_SIZE  ((size_t)1024)

#define FAST_SHMEM_STORAGE_SIZE sizeof(struct storage)

#define FAST_SHMEM_INVALID_SPLIT_THRESHOLD       (0)
#define FAST_SHMEM_DEFAULT_SPLIT_THRESHOLD       (64)
#define FAST_SHMEM_MEM_LEVEL                     (32)
#define FAST_SHMEM_LEVEL_TYPE_LINEAR             (0)
#define FAST_SHMEM_LEVEL_TYPE_EXP                (1)
#define FAST_SHMEM_EXP_FACTOR                     2
#define FAST_SHMEM_LINEAR_FACTOR                  1024

#define DEFAULT_ALIGNMENT_MASK  (1)
#define DEFAULT_ALIGNMENT_SIZE  (1<<DEFAULT_ALIGNMENT_MASK)

#define FAST_SHMEM_ERROR                    (-1)
#define FAST_SHMEM_OK                       (0)


enum {
    FAST_SHMEM_ERR_NONE = 0,

    FAST_SHMEM_ERR_START = 100,

    FAST_SHMEM_ERR_CREATE_SIZE,
    FAST_SHMEM_ERR_CREATE_MINSIZE,
    FAST_SHMEM_ERR_CREATE_LEVELTYPE,
    FAST_SHMEM_ERR_CREATE_FREELEN,
    FAST_SHMEM_ERR_CREATE_TOTALSIZE,
    FAST_SHMEM_ERR_CREATE_TOTALSIZE_NOT_ENOUGH,
    FAST_SHMEM_ERR_CREATE_MMAP,
    FAST_SHMEM_ERR_CREATE_STORAGESIZE,

    FAST_SHMEM_ERR_RELEASE_NULL,
    FAST_SHMEM_ERR_RELEASE_MUNMAP,

    FAST_SHMEM_ERR_ALLOC_PARAM,
    FAST_SHMEM_ERR_ALLOC_EXHAUSTED,
    FAST_SHMEM_ERR_ALLOC_MAX_AVAILABLE_EMPTY,
    FAST_SHMEM_ERR_ALLOC_NO_AVAILABLE_FREE_LIST,
    FAST_SHMEM_ERR_ALLOC_NO_FIXED_FREE_SPACE,
    FAST_SHMEM_ERR_ALLOC_FOUND_NO_FIXED,
    FAST_SHMEM_ERR_ALLOC_REMOVE_FREE,

    FAST_SHMEM_ERR_GET_MAX_PARAM,
    FAST_SHMEM_ERR_GET_MAX_EXHAUSTED,
    FAST_SHMEM_ERR_GET_MAX_CRITICAL,

    FAST_SHMEM_ERR_SPLIT_ALLOC_PARAM,
    FAST_SHMEM_ERR_SPLIT_ALLOC_NO_FIXED_REQ_MINSIZE,
    FAST_SHMEM_ERR_SPLIT_ALLOC_REMOVE_FREE,

    FAST_SHMEM_ERR_FREE_PARAM,
    FAST_SHMEM_ERR_FREE_NONALLOCED,
    FAST_SHMEM_ERR_FREE_REMOVE_NEXT,
    FAST_SHMEM_ERR_FREE_REMOVE_PREV,

    FAST_SHMEM_ERR_END
};

struct storage;
struct storage{
    queue_t              order_entry;
    queue_t              free_entry;
    unsigned int             alloc;
    size_t                   size;
    size_t                   act_size;
    queue_t             *free_list_head;
};

struct free_node{
    queue_t              available_entry;
    queue_t              free_list_head; 
    unsigned int             index;
};

typedef struct fast_shmem_stat_s {
    size_t                   used_size; //not include system and storage size
    size_t                   reqs_size;
    size_t                   st_count;
    size_t                   st_size;
    size_t                   total_size;
    size_t                   system_size;
    size_t                   failed;
    size_t                   split_failed;
    size_t                   split;
} fast_shmem_stat_t;

typedef struct fast_shmem_s {
    queue_t                  order;  //all block list include free and alloced
    struct free_node        *free;   //free array from 0 to free_len - 1
    size_t                   free_len; //length of free array
    queue_t                  available; //the list of non-empty free_list
    size_t                   max_available_index; //the id of last element in available
    size_t                   min_size; // the size of free[0]
    size_t                   max_size; // the size of free[free_len - 1]
    size_t                   factor;  //increament step
    size_t                   split_threshold; //threshold size for split default to min_size
    int                      level_type;//increament type                    
    fast_shmem_stat_t         shmem_stat;// stat for shmem
} fast_shmem_t;

fast_shmem_t*
fast_shmem_create(size_t size, size_t min_size, size_t max_size,
    int level_type, size_t factor, unsigned int *shmem_errno);

int
fast_shmem_release(fast_shmem_t **shm, unsigned int *shmem_errno);   

inline size_t fast_shmem_set_alignment_size(size_t size);
inline size_t fast_shmem_get_alignment_size();

void*
fast_shmem_alloc(fast_shmem_t *shm, size_t size, unsigned int *shmem_errno);
void*
fast_shmem_calloc(fast_shmem_t *shm, size_t size, unsigned int *shmem_errno);
void
fast_shmem_free(fast_shmem_t *shm, void *addr, unsigned int *shmem_errno);
void*
fast_shmem_split_alloc(fast_shmem_t *shm, size_t *act_size,
    size_t minsize, unsigned int *shmem_errno);

void           fast_shmem_dump(fast_shmem_t *shm, FILE *out);
size_t         fast_shmem_get_used_size(fast_shmem_t *shm);
size_t         fast_shmem_get_total_size(fast_shmem_t *shm);
size_t         fast_shmem_get_system_size(fast_shmem_t *shm);
inline int     fast_shmem_set_split_threshold(fast_shmem_t *shm, size_t size);

const char*
fast_shmem_strerror(unsigned int shmem_errno);
int
fast_shmem_get_stat(fast_shmem_t *shmem, fast_shmem_stat_t *stat);

uchar_t *fast_shm_strdup(fast_shmem_t *shm, string_t *str);

#endif
