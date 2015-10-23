#include "fast_types.h"
#include "fast_memory.h"
#include "fast_commpool.h"


static int mpool_mgmt_allocator_init(fast_mem_allocator_t *this,
        void *param_data)
{
    struct mpool_mgmt_param_s  *param = param_data;
    mpool_mgmt_t           *pool;
    
    if (!param) {
        return FAST_MEM_ALLOCATOR_ERROR;
    }

    pool = mpool_mgmt_create(param);
    if (!pool) {
        return FAST_MEM_ALLOCATOR_ERROR;
    }

    this->private_data = pool;

    return FAST_MEM_ALLOCATOR_OK;
}

static int mpool_mgmt_allocator_release(fast_mem_allocator_t *this, 
        void *param_data)
{
    (void)param_data;

    mpool_mgmt_t *pool  = this->private_data;
 
    if (!pool) {
        return FAST_MEM_ALLOCATOR_ERROR;
    }

    destroy_mpool_mgmt(pool);
    
    return FAST_MEM_ALLOCATOR_OK;
}


static void * mpool_mgmt_allocator_alloc(fast_mem_allocator_t *this,
        size_t size, void *param_data)
{
    (void)param_data;

    mpool_mgmt_t *pool = this->private_data;

    if (!pool) {
        return NULL;
    }

    return mpool_alloc(pool, size);
}



static void * mpool_mgmt_allocator_calloc(fast_mem_allocator_t *this,
        size_t size, void *param_data)
{
    void *ptr;
    if (!this) {
        return NULL;
    }
    ptr = this->alloc(this, size, param_data);
    if (ptr) {
        memory_zero(ptr, size);
    }
    return ptr;
}


static int mpool_mgmt_allocator_free(fast_mem_allocator_t *this,
        void *ptr, void *param_data) {

        (void)this;
        (void)ptr;
        (void)param_data;

        return FAST_MEM_ALLOCATOR_OK;
}




static  fast_mem_allocator_t fast_mpool_mgmt = {
    .private_data   = NULL,
    .type           = FAST_MEM_ALLOCATOR_TYPE_COMMPOOL,
    .init           = mpool_mgmt_allocator_init,
    .release        = mpool_mgmt_allocator_release,
    .alloc          = mpool_mgmt_allocator_alloc,
    .calloc         = mpool_mgmt_allocator_calloc,
    .free           = mpool_mgmt_allocator_free,

};


const fast_mem_allocator_t *fast_get_commpool_allocator(void)
{
    return &fast_mpool_mgmt;
}




