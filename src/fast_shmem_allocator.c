#include "fast_shmem_allocator.h"
#include "fast_shmem.h"
#include "fast_memory.h"


static int
fast_shmem_allocator_init(fast_mem_allocator_t *this, void *param_data);

static int
fast_shmem_allocator_release(fast_mem_allocator_t *this, void *param_data);

static void *
fast_shmem_allocator_alloc(fast_mem_allocator_t *this, size_t size,
    void *param_data);

static void *
fast_shmem_allocator_calloc(fast_mem_allocator_t *this, size_t size,
    void *param_data);

static void *
fast_shmem_allocator_split_alloc(fast_mem_allocator_t *this, size_t *act_size,
    size_t req_minsize, void *param_data);

static int 
fast_shmem_allocator_free(fast_mem_allocator_t *this, void *ptr,
    void *param_data);

static const char *
fast_shmem_allocator_strerror(fast_mem_allocator_t *this, void *err_data);

static int
fast_shmem_allocator_stat(fast_mem_allocator_t *this, void *stat_data);

static fast_mem_allocator_t fast_shmem_allocator = {
    .private_data   = NULL,
    .type           = FAST_MEM_ALLOCATOR_TYPE_SHMEM,
    .init           = fast_shmem_allocator_init,
    .release        = fast_shmem_allocator_release,
    .alloc          = fast_shmem_allocator_alloc,
    .calloc         = fast_shmem_allocator_calloc,
    .split_alloc    = fast_shmem_allocator_split_alloc,
    .free           = fast_shmem_allocator_free,
    .strerror       = fast_shmem_allocator_strerror,
    .stat           = fast_shmem_allocator_stat
};

static int
fast_shmem_allocator_init(fast_mem_allocator_t *this, void *param_data)
{
    fast_shmem_allocator_param_t *param =
         (fast_shmem_allocator_param_t *)param_data;
         
    if (!this || !param) {
        return FAST_MEM_ALLOCATOR_ERROR; 
    }
    if (this->private_data) {
        return FAST_MEM_ALLOCATOR_OK;
    }
    
    this->private_data = fast_shmem_create(param->size, param->min_size,
         param->max_size, param->level_type, param->factor, &param->err_no); 
    if (!this->private_data) {
        return FAST_MEM_ALLOCATOR_ERROR;
    }
    return FAST_MEM_ALLOCATOR_OK;
}

static int
fast_shmem_allocator_release(fast_mem_allocator_t *this, void *param_data)
{
    fast_shmem_allocator_param_t  *param;
    fast_shmem_t                 **shmem;

    param = (fast_shmem_allocator_param_t *)param_data;

    if (!this || !this->private_data) {
        return FAST_MEM_ALLOCATOR_ERROR;
    }
    
    shmem = (fast_shmem_t **)&this->private_data;
    if (fast_shmem_release(shmem, &param->err_no) == FAST_SHMEM_ERROR) {
        return FAST_MEM_ALLOCATOR_ERROR;
    }
    return FAST_MEM_ALLOCATOR_OK;
}

static void *
fast_shmem_allocator_alloc(fast_mem_allocator_t *this, size_t size,
    void *param_data)
{
    unsigned int  shmem_errno;
    fast_shmem_t  *shmem;
    void         *ptr;

    if (!this || !this->private_data) {
        return NULL;
    }
    shmem = (fast_shmem_t *)this->private_data;
    ptr = fast_shmem_alloc(shmem, size, &shmem_errno);
    if (!ptr) {
        if (param_data) {
            *(unsigned int *)param_data = shmem_errno;
        }
    }
    return ptr;
}

static void *
fast_shmem_allocator_calloc(fast_mem_allocator_t *this, size_t size, void *param_data)
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

static void *
fast_shmem_allocator_split_alloc(fast_mem_allocator_t *this, size_t *act_size,
    size_t req_minsize, void *param_data)
{
    fast_shmem_t                       *shmem;
    unsigned int                      *shmem_errno;

    if (!this || !this->private_data || !param_data) {
        return NULL;
    }
    shmem = (fast_shmem_t *)this->private_data;
    shmem_errno = (unsigned int *)param_data;

    return fast_shmem_split_alloc(shmem, act_size, req_minsize, shmem_errno);
}

static int 
fast_shmem_allocator_free(fast_mem_allocator_t *this, void *ptr, void *param_data)
{
    unsigned int     shmem_errno;
    fast_shmem_t     *shmem;  

    if (!this || !this->private_data || !ptr) {
        return FAST_MEM_ALLOCATOR_ERROR;
    }

    shmem = (fast_shmem_t *)this->private_data;
    fast_shmem_free(shmem, ptr, &shmem_errno);

    if (shmem_errno != FAST_SHMEM_ERR_NONE) {
        if (param_data) {
            *(unsigned int *)param_data = shmem_errno;
        }
        return FAST_MEM_ALLOCATOR_ERROR;
    }
    return FAST_MEM_ALLOCATOR_OK;
}

static const char *
fast_shmem_allocator_strerror(fast_mem_allocator_t *this, void *err_data)
{
    if (!this || !err_data) {
        return NULL;
    }
    return fast_shmem_strerror(*(unsigned int *)err_data); 
}

static int
fast_shmem_allocator_stat(fast_mem_allocator_t *this, void *stat_data)
{
    fast_shmem_stat_t *stat = (fast_shmem_stat_t *)stat_data;
    fast_shmem_t      *shmem;

    if (!this || !this->private_data || !stat) {
        return FAST_MEM_ALLOCATOR_ERROR;
    }
   
    shmem = (fast_shmem_t *)this->private_data;
    if (fast_shmem_get_stat(shmem, stat) == FAST_SHMEM_ERROR) {
        return FAST_MEM_ALLOCATOR_ERROR;
    }
    return FAST_MEM_ALLOCATOR_OK;
}

const fast_mem_allocator_t *fast_get_shmem_allocator(void)
{
    return &fast_shmem_allocator;
}


