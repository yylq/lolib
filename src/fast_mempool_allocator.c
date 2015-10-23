#include "fast_mempool_allocator.h"
#include "fast_memory_pool.h"
#include "fast_memory.h"

static int
fast_mempool_allocator_init(fast_mem_allocator_t *this, void *param_data);

static int
fast_mempool_allocator_release(fast_mem_allocator_t *this, void *param_data);

static void * 
fast_mempool_allocator_alloc(fast_mem_allocator_t *this, size_t size,
    void *param_data);

static void *
fast_mempool_allocator_calloc(fast_mem_allocator_t *this, size_t size,
    void *param_data);

static const char *
fast_mempool_allocator_strerror(fast_mem_allocator_t *this, void *err_data);

static fast_mem_allocator_t fast_mempool_allocator = {
    .private_data       = NULL,
    .type               = FAST_MEM_ALLOCATOR_TYPE_MEMPOOL,
    .init               = fast_mempool_allocator_init,
    .release            = fast_mempool_allocator_release,
    .alloc              = fast_mempool_allocator_alloc,
    .calloc             = fast_mempool_allocator_calloc,
    .free               = NULL,
    .strerror           = fast_mempool_allocator_strerror,
    .stat               = NULL
};


static int
fast_mempool_allocator_init(fast_mem_allocator_t *this, void *param_data)
{
    fast_mempool_allocator_param_t *param;

    if (!this || !param_data) {
        return FAST_MEM_ALLOCATOR_ERROR;
    }
    if (this->private_data) {
        return FAST_MEM_ALLOCATOR_OK;
    }
    param = (fast_mempool_allocator_param_t *)param_data;
    this->private_data = pool_create(param->pool_size, param->max_size, 
        this->log);
    if (!this->private_data) {
        return FAST_MEM_ALLOCATOR_ERROR;
    }
    return FAST_MEM_ALLOCATOR_OK;
}

static int
fast_mempool_allocator_release(fast_mem_allocator_t *this, void *param_data)
{
    pool_t              *pool;

    if (!this || !this->private_data) {
        return FAST_MEM_ALLOCATOR_ERROR;
    }
    pool = (pool_t *)this->private_data;
    pool_destroy(pool);
    this->private_data = NULL;

    return FAST_MEM_ALLOCATOR_OK;
}

static void *
fast_mempool_allocator_alloc(fast_mem_allocator_t *this, size_t size,
    void *param_data)
{
    pool_t *pool;

    if (!this || !this->private_data) {
        return NULL;
    }

    pool = (pool_t *)this->private_data;

    return pool_alloc(pool, size);
}

static void *
fast_mempool_allocator_calloc(fast_mem_allocator_t *this, size_t size,
    void *param_data)
{
    void *ptr;
    ptr = this->alloc(this, size, param_data); 
    if (ptr) {
        memory_zero(ptr, size);
    }
    return ptr;
}

static const char *
fast_mempool_allocator_strerror(fast_mem_allocator_t *this, void *err_data)
{
    return "no more error info";
}

const fast_mem_allocator_t *fast_get_mempool_allocator(void)
{
    return &fast_mempool_allocator;
}
