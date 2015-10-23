#ifndef _FAST_PROCESS_LOCK
#define _FAST_PROCESS_LOCK
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>

#include "fast_mem_allocator.h"
#include "fast_memory_pool.h"

#define	FAST_LOCK_ERROR  -1
#define FAST_LOCK_OK     0
#define	FAST_LOCK_OFF    1
#define	FAST_LOCK_ON     2

#define FAST_LOCK_FALSE  0
#define FAST_LOCK_TRUE   1
/*
#if defined(__GNUC__) && defined(__x86_64__)
#define fcache_atomic_t int64_t
#elif defined(__GNUC__)
#define fcache_atomic_t int32_t
#endif
*/

#if (__GNUC__ >= 4 && __GNUC_MINOR__ >=2 )
#define CAS(val, old, set) \
    __sync_bool_compare_and_swap((val), (old), (set))
#else
#define CAS(val, old, set) \
	cmp_and_swap((val), (old), (set))
#endif

enum {
    FAST_LOCK_ERR_NONE = 0,

    FAST_LOCK_ERR_START = 100,

    FAST_LOCK_ERR_PARAM,

    FAST_LOCK_ERR_ALLOCATOR_ERROR,

    FAST_LOCK_ERR_SYSCALL_MUTEXATTR_INIT,
    FAST_LOCK_ERR_SYSCALL_MUTEXATTR_SETPSHARED,
    FAST_LOCK_ERR_SYSCALL_MUTEX_INIT,
    FAST_LOCK_ERR_SYSCALL_MUTEXATTR_DESTROY,
    FAST_LOCK_ERR_SYSCALL_MUTEX_DESTROY,
    FAST_LOCK_ERR_SYSCALL_MUTEX_LOCK,
    FAST_LOCK_ERR_SYSCALL_MUTEX_TRYLOCK,
    FAST_LOCK_ERR_SYSCALL_MUTEX_UNLOCK,

    FAST_LOCK_ERR_SYSCALL_SIGPROCMASK,

    FAST_LOCK_ERR_SYSCALL_RWLOCKATTR_INIT,
    FAST_LOCK_ERR_SYSCALL_RWLOCKATTR_SETPSHARED,
    FAST_LOCK_ERR_SYSCALL_RWLOCK_INIT,
    FAST_LOCK_ERR_SYSCALL_RWLOCKATTR_DESTROY,
    FAST_LOCK_ERR_SYSCALL_RWLOCK_DESTROY,
    FAST_LOCK_ERR_SYSCALL_RWLOCK_RDLOCK,
    FAST_LOCK_ERR_SYSCALL_RWLOCK_UNLOCK,
    FAST_LOCK_ERR_SYSCALL_RWLOCK_WRLOCK,
    FAST_LOCK_ERR_SYSCALL_RWLOCK_TRYWRLOCK,

    FAST_LOCK_ERR_END
};

typedef struct {
	pthread_mutex_t          lock;
	pthread_mutexattr_t      mutex_attr;
    sigset_t                 sig_block_mask;
    sigset_t                 sig_prev_mask;
	fast_mem_allocator_t     *allocator;
} fast_process_lock_t;

typedef struct {
	pthread_rwlock_t         rwlock;
	pthread_rwlockattr_t     rwlock_attr;
	sigset_t                 sig_block_mask;
	sigset_t                 sig_prev_mask;
	fast_mem_allocator_t     *allocator;
} fast_process_rwlock_t;

typedef struct {
    volatile uint64_t        lock;
	fast_mem_allocator_t     *allocator;
} fast_atomic_lock_t;

typedef struct {
    int                      syscall_errno;
    unsigned int             lock_errno;
    unsigned int             allocator_errno;
    fast_mem_allocator_t     *err_allocator;
} fast_lock_errno_t;

//int lock_init(fast_mem_allocator_t *allocator);

fast_process_lock_t *
fast_process_lock_create(fast_mem_allocator_t *allocator,
    fast_lock_errno_t *error);
int
fast_process_lock_release(fast_process_lock_t *process_lock,
    fast_lock_errno_t *error);
int
fast_process_lock_reset(fast_process_lock_t *process_lock,
    fast_lock_errno_t *error);
int
fast_process_lock_on(fast_process_lock_t *process_lock,
    fast_lock_errno_t *error);
int
fast_process_lock_off(fast_process_lock_t *process_lock,
    fast_lock_errno_t *error);
int
fast_process_lock_try_on(fast_process_lock_t *process_lock,
    fast_lock_errno_t *error);


fast_process_rwlock_t *
fast_process_rwlock_create(fast_mem_allocator_t *allocator,
    fast_lock_errno_t *error);
int
fast_process_rwlock_release(fast_process_rwlock_t *process_rwlock,
    fast_lock_errno_t *error);
int 
fast_process_rwlock_init(fast_process_rwlock_t * process_rwlock,
    fast_lock_errno_t *error);
int fast_process_rwlock_destroy(fast_process_rwlock_t* process_rwlock,
    fast_lock_errno_t *error);

int
fast_process_rwlock_reset(fast_process_rwlock_t *process_rwlock,
    fast_lock_errno_t *error);
int
fast_process_rwlock_read_on(fast_process_rwlock_t *process_rwlock,
    fast_lock_errno_t *error);
int
fast_process_rwlock_write_on(fast_process_rwlock_t *process_rwlock,
    fast_lock_errno_t *error);
int
fast_process_rwlock_write_try_on(fast_process_rwlock_t *process_rwlock,
    fast_lock_errno_t *error);
int
fast_process_rwlock_off(fast_process_rwlock_t *process_rwlock,
    fast_lock_errno_t *error);


fast_atomic_lock_t *
fast_atomic_lock_create(fast_mem_allocator_t *allocator,
    fast_lock_errno_t *error);
int fast_atomic_lock_init(fast_atomic_lock_t *atomic_lock);

int
fast_atomic_lock_release(fast_atomic_lock_t *atomic_lock,
    fast_lock_errno_t *error);
int
fast_atomic_lock_reset(fast_atomic_lock_t *atomic_lock,
    fast_lock_errno_t *error);
int
fast_atomic_lock_try_on(fast_atomic_lock_t *atomic_lock,
    fast_lock_errno_t *error);
int
fast_atomic_lock_on(fast_atomic_lock_t *atomic_lock,
    fast_lock_errno_t *error);
int
fast_atomic_lock_off(fast_atomic_lock_t *atomic_lock,
    fast_lock_errno_t *error);

const char *
fast_lock_strerror(fast_lock_errno_t *error);

inline void fast_atomic_lock_off_force(fast_atomic_lock_t *atomic_lock);

fast_atomic_lock_t *fast_atomic_lock_pool_create(pool_t *pool);

#endif

