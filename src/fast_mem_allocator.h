#ifndef _FAST_MEM_ALLOCATOR_H
#define _FAST_MEM_ALLOCATOR_H
#include <stdlib.h>
#include "fast_error_log.h"
typedef struct fast_mem_allocator_s fast_mem_allocator_t;

//init
typedef int   (*fast_init_ptr_t)     (fast_mem_allocator_t *this, void *param_data);
//release
typedef int   (*fast_release_ptr_t)  (fast_mem_allocator_t *this, void *param_data);

//alloc
typedef void* (*fast_alloc_ptr_t)    (fast_mem_allocator_t *this, size_t size,
    void *param_data);
//calloc
typedef void* (*fast_calloc_ptr_t)   (fast_mem_allocator_t *this, size_t size,
    void *param_data);
typedef void* (*fast_split_alloc_ptr_t) (fast_mem_allocator_t *this, size_t *act_size,
    size_t req_minsize, void *param_data);
//free
typedef int  (*fast_free_ptr_t)     (fast_mem_allocator_t *this, void *ptr,
    void *param_data);
//strerror
typedef const char* (*fast_strerror_ptr_t) (fast_mem_allocator_t *this,
    void *err_data);

typedef int (*fast_stat_ptr_t) (fast_mem_allocator_t *this, void *stat_data);

struct fast_mem_allocator_s {
    void                 *private_data;
    int                   type;
    fast_init_ptr_t        init;
    fast_release_ptr_t     release;
    fast_alloc_ptr_t       alloc;
    fast_calloc_ptr_t      calloc;
    fast_split_alloc_ptr_t split_alloc;
    fast_free_ptr_t        free;
    fast_strerror_ptr_t    strerror;
    fast_stat_ptr_t        stat;
    log_t                  *log;
};

enum {
    FAST_MEM_ALLOCATOR_TYPE_SHMEM = 1,
    FAST_MEM_ALLOCATOR_TYPE_MEMPOOL,
    FAST_MEM_ALLOCATOR_TYPE_COMMPOOL,
};

#define FAST_MEM_ALLOCATOR_OK    (0)
#define FAST_MEM_ALLOCATOR_ERROR (-1)


extern const fast_mem_allocator_t *fast_get_mempool_allocator(void);
extern const fast_mem_allocator_t *fast_get_shmem_allocator(void);
extern const fast_mem_allocator_t *fast_get_commpool_allocator(void);

fast_mem_allocator_t *fast_mem_allocator_new(int allocator_type);
fast_mem_allocator_t *
fast_mem_allocator_new_init(int allocator_type, void *init_param);

void fast_mem_allocator_delete(fast_mem_allocator_t *allocator);
#endif

