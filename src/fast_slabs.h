#ifndef _FAST_SLABS_H
#define _FAST_SLABS_H

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "fast_shmem_allocator.h"

//#define FAST_SLAB_DEFAULT_MAX_SIZE                 ((size_t)10<<20) //10MB
#define FAST_SLAB_DEFAULT_MAX_SIZE                 ((size_t)10485760) //10MB
#define FAST_SLAB_DEFAULT_MIN_SIZE                 ((size_t)1024)     //KB
#define FAST_SLAB_MAX_SIZE_PADDING                 ((size_t)1<<20)    //MB

enum {
    FAST_SLAB_ERR_NONE = 0,
    //for slab
    FAST_SLAB_ERR_START = 100,

    FAST_SLAB_ERR_CREATE_PARAM,
    FAST_SLAB_ERR_CREATE_POWER_FACTOR,
    FAST_SLAB_ERR_CREATE_LINER_FACTOR,
    FAST_SLAB_ERR_CREATE_UPTYPE,

    FAST_SLAB_ERR_RELEASE_PARAM,

    FAST_SLAB_ERR_ALLOC_PARAM,
    FAST_SLAB_ERR_ALLOC_INVALID_ID,
    FAST_SLAB_ERR_ALLOC_FAILED,

    FAST_SLAB_ERR_SPLIT_ALLOC_PARAM,
    FAST_SLAB_ERR_SPLIT_ALLOC_NOT_SUPPORTED,
    FAST_SLAB_ERR_SPLIT_ALLOC_CHUNK_SIZE_TOO_LARGE,

    FAST_SLAB_ERR_FREE_PARAM,
    FAST_SLAB_ERR_FREE_NOT_SUPPORTED,
    FAST_SLAB_ERR_FREE_CHUNK_ID,
    
    FAST_SLAB_ERR_ALLOCATOR_ERROR,

    FAST_SLAB_ERR_END

};
#define FAST_SLAB_NO_ERROR                          0
#define FAST_SLAB_ERROR_INVALID_ID                 -1
#define FAST_SLAB_ERROR_NOSPACE                    -2
#define FAST_SLAB_ERROR_UNKNOWN                    -3
#define FAST_SLAB_ERROR_ALLOC_FAILED               -4
#define FAST_SLAB_SPLIT_ID                         -5

#define	FAST_SLAB_FALSE	            0
#define	FAST_SLAB_TRUE	            1
#define FAST_SLAB_NO_FREE            2

#define FAST_SLAB_ERROR              -1
#define FAST_SLAB_OK                 0

#define FAST_SLAB_RECOVER_FACTOR     2

#define FAST_SLAB_ALLOC_TYPE_REQ     0
#define FAST_SLAB_ALLOC_TYPE_ACT     1

#define FAST_SLAB_CHUNK_SIZE    sizeof(chunk_link_t)

enum {
    FAST_SLAB_UPTYPE_POWER = 1,
    FAST_SLAB_UPTYPE_LINEAR
};

enum {
    FAST_SLAB_POWER_FACTOR = 2,
    FAST_SLAB_LINEAR_FACTOR = 1024,
};

typedef struct chunk_link chunk_link_t;

struct chunk_link {
	size_t                  size;
    size_t                  req_size;
    ssize_t                 id;
    chunk_link_t           *next;
};

typedef struct slabclass_s {
    size_t                  size;
    chunk_link_t           *free_list;
} slabclass_t;

typedef struct fast_slab_errno_s {
    unsigned int            slab_errno;
    unsigned int            allocator_errno;
} fast_slab_errno_t;

typedef struct fast_slab_stat_s{
    size_t                  used_size;
    size_t                  reqs_size;
    size_t                  free_size;
    size_t                  chunk_count;
    size_t                  chunk_size;
    size_t                  system_size;
    size_t                  failed;
    size_t                  recover;
    size_t                  recover_failed;
    size_t                  split_failed;
} fast_slab_stat_t;

typedef struct fast_slab_manager_s {
    int                     uptype;    //increament type
    ssize_t                 free_len;  //length of array slabclass
    size_t                  factor;    //increament step
    size_t                  min_size;  //the size of slabclass[0]
    fast_slab_stat_t         slab_stat; //stat for slab
    fast_mem_allocator_t    *allocator; //the pointer to memory
    slabclass_t            *slabclass; // slab class array 0..free_len - 1
} fast_slab_manager_t;

fast_slab_manager_t *
fast_slabs_create(fast_mem_allocator_t *allocator, int uptype, size_t factor,
    const size_t item_size_min, const size_t item_size_max,
    fast_slab_errno_t *err_no);

int
fast_slabs_release(fast_slab_manager_t **slab_mgr, fast_slab_errno_t *err_no);

void *
fast_slabs_alloc(fast_slab_manager_t *slab_mgr, int alloc_type, 
    size_t req_size, size_t *slab_size, fast_slab_errno_t *err_no);

void *
fast_slabs_split_alloc(fast_slab_manager_t *slab_mgr, size_t req_size,
    size_t *slab_size, size_t req_minsize, fast_slab_errno_t *err_no);

int
fast_slabs_free(fast_slab_manager_t *slab_mgr, void *ptr, fast_slab_errno_t *err_no);

const char *
fast_slabs_strerror(fast_slab_manager_t *slab_mgr, const fast_slab_errno_t *err_no);

int
fast_slabs_get_stat(fast_slab_manager_t *slab_mgr, fast_slab_stat_t *stat);

size_t
fast_slabs_get_chunk_size(fast_slab_manager_t *slab_mgr);
#endif

