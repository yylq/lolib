#ifndef _FAST_SHMEM_ALLOCATOR_H
#define _FAST_SHMEM_ALLOCATOR_H

#include "fast_mem_allocator.h"

typedef struct fast_shmem_allocator_param_s {
    size_t          size;
    size_t          min_size;
    size_t          max_size;
    size_t          factor;
    int             level_type;
    unsigned int    err_no;

} fast_shmem_allocator_param_t;

#endif
