#include "fast_mem_allocator.h"
#include "fast_shmem_allocator.h"
#include "fast_mempool_allocator.h"

fast_mem_allocator_t *fast_mem_allocator_new(int allocator_type)
{
    fast_mem_allocator_t *allocator;
    allocator = malloc(sizeof(fast_mem_allocator_t));
    if (!allocator) {
        return NULL;
    }
    switch (allocator_type) {
        case FAST_MEM_ALLOCATOR_TYPE_SHMEM:
            *allocator = *fast_get_shmem_allocator();
            break;
        case FAST_MEM_ALLOCATOR_TYPE_MEMPOOL:
            *allocator = *fast_get_mempool_allocator();
            break;
       case FAST_MEM_ALLOCATOR_TYPE_COMMPOOL:
            *allocator = *fast_get_commpool_allocator();
 
        default:
            return NULL;
    }
    allocator->private_data = NULL;

    return allocator;
}

fast_mem_allocator_t *fast_mem_allocator_new_init(int allocator_type,
    void *init_param)
{
    fast_mem_allocator_t             *allocator;

    allocator = malloc(sizeof(fast_mem_allocator_t));
    if (!allocator) {
        return NULL;
    }
    switch (allocator_type) {
        case FAST_MEM_ALLOCATOR_TYPE_SHMEM:
            *allocator = *fast_get_shmem_allocator();
            break;
        case FAST_MEM_ALLOCATOR_TYPE_MEMPOOL:
            *allocator = *fast_get_mempool_allocator();
            break;
        case FAST_MEM_ALLOCATOR_TYPE_COMMPOOL:
            *allocator = *fast_get_commpool_allocator();
            break;
        default:
            return NULL;
    }
    allocator->private_data = NULL;
    if (init_param && allocator->init(allocator, init_param)
        == FAST_MEM_ALLOCATOR_ERROR) {
        return NULL;
    }
    return allocator;
}

void fast_mem_allocator_delete(fast_mem_allocator_t *allocator)
{
    fast_shmem_allocator_param_t     shmem;
    fast_mempool_allocator_param_t   mempool;

    if (!allocator) {
        return;
    }
    if (allocator->private_data) {
        switch (allocator->type) {
            case FAST_MEM_ALLOCATOR_TYPE_SHMEM:
                allocator->release(allocator, &shmem);
                break;
            case FAST_MEM_ALLOCATOR_TYPE_MEMPOOL:
                allocator->release(allocator, &mempool);
                break;
            case FAST_MEM_ALLOCATOR_TYPE_COMMPOOL:
                allocator->release(allocator, NULL);
                break;
        }
    }

    free(allocator);
}
