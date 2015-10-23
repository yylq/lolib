#ifndef _FAST_MEMPOOL_ALLOCATOR_H
#define _FAST_MEMPOOL_ALLOCATOR_H

#include "fast_mem_allocator.h"
typedef struct fast_mempool_allocator_param_s {
    size_t pool_size;
    size_t max_size;
} fast_mempool_allocator_param_t;
#endif

