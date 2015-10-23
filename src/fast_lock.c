#include <errno.h>

#include "fast_lock.h"

#define FAST_LOCK_ERRNO_CLEAN(e)                \
    (e)->lock_errno = FAST_LOCK_ERR_NONE;       \
    (e)->syscall_errno = 0;                    \
    (e)->allocator_errno = FAST_LOCK_ERR_NONE;  \
    (e)->err_allocator = NULL

#define FAST_LOCK_ERRNO_ALLOCATOR(e, a)         \
    (e)->err_allocator = (a)


/*Special Note:
    When the instances of fast_process_lock are more than one
    the fast_process_lock_held assignment must be confused!!!!!!
*/

static const char *fast_lock_err_string[] = {
    "fast_lock: unknown error number",

    "fast_lock: parameter error",
    
    "fast_lock: allocator error",
    "fast_lock: mutex attribute init error",
    "fast_lock: mutex attribute setpshared error",
    "fast_lock: mutex init error",
    "fast_lock: mutex attribute destroy error",
    "fast_lock: mutex destroy error",
    "fast_lock: mutex lock error", 
    "fast_lock: mutex try lock error",
    "fast_lock: mutex unlock error",

    "fast_lock: sigprocmask error",

    "fast_lock: rwlock attribute init error",
    "fast_lock: rwlock attribute setpshared error",
    "fast_lock: rwlock init error",
    "fast_lock: rwlock attribute destroy error",
    "fast_lock: rwlock destroy error",
    "fast_lock: rwlock reading lock error",
    "fast_lock: rwlock unlock error",
    "fast_lock: rwlock writing lock error",
    "fast_lock: rwlock writing try lock error",
    NULL
};

static inline unsigned char cmp_and_swap(volatile uint64_t *ptr,
    uint64_t old, uint64_t nw)
{
    unsigned char ret;
    __asm__ __volatile__("lock; cmpxchgq %1,%2; setz %0"
        : "=a"(ret)
        : "q"(nw), "m"(*(volatile uint64_t *)(ptr)), "0"(old)
        : "memory");
    return ret;
}

fast_process_lock_t *
fast_process_lock_create(fast_mem_allocator_t *allocator,
    fast_lock_errno_t *error)
{
    fast_process_lock_t *process_lock = NULL;

    if (!error) {
        return NULL;
    }

    FAST_LOCK_ERRNO_CLEAN(error);
    if (!allocator) {
        error->lock_errno = FAST_LOCK_ERR_PARAM;
        return NULL;
    }
    //FAST_LOCK_ERRNO_ALLOCATOR(error, allocator);

    process_lock = allocator->alloc(allocator, sizeof(fast_process_lock_t),
        &error->allocator_errno);
    if (!process_lock) {
        error->lock_errno = FAST_LOCK_ERR_ALLOCATOR_ERROR;
        return NULL;
    }
    process_lock->allocator = allocator;
   	//init mutex attribute
    if ((error->syscall_errno =
             pthread_mutexattr_init(&process_lock->mutex_attr))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_MUTEXATTR_INIT;
        return NULL;
    }

    //set mutex attribute to process shared
    if ((error->syscall_errno =
            pthread_mutexattr_setpshared(&process_lock->mutex_attr,
    	    PTHREAD_PROCESS_SHARED))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_MUTEXATTR_SETPSHARED;
        return NULL;
    }

    if ((error->syscall_errno = pthread_mutex_init(&process_lock->lock, 
        &process_lock->mutex_attr))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_MUTEX_INIT;
        return NULL;
    }

    //remove SIGHUP SIGTERM SIGUSR1 from sigset of currnet process
    sigfillset(&process_lock->sig_block_mask);
    sigdelset(&process_lock->sig_block_mask, SIGALRM);
    sigdelset(&process_lock->sig_block_mask, SIGINT);
    sigdelset(&process_lock->sig_block_mask, SIGCHLD);
    sigdelset(&process_lock->sig_block_mask, SIGPIPE);
    sigdelset(&process_lock->sig_block_mask, SIGSEGV);
    sigdelset(&process_lock->sig_block_mask, SIGHUP);
    sigdelset(&process_lock->sig_block_mask, SIGQUIT);
    sigdelset(&process_lock->sig_block_mask, SIGTERM);
    sigdelset(&process_lock->sig_block_mask, SIGIO);
    sigdelset(&process_lock->sig_block_mask, SIGUSR1);

    return process_lock;    
}

int fast_process_lock_release(fast_process_lock_t *process_lock,
    fast_lock_errno_t *error)
{
    fast_mem_allocator_t *allocator; 

    if (!error) {
        return FAST_LOCK_ERROR;
    }

    FAST_LOCK_ERRNO_CLEAN(error);
	if (!process_lock || !process_lock->allocator) {
        error->lock_errno = FAST_LOCK_ERR_PARAM;
		return FAST_LOCK_ERROR;
	}
    FAST_LOCK_ERRNO_ALLOCATOR(error, process_lock->allocator);
	//recover signal mask
    if (sigprocmask(SIG_SETMASK, &process_lock->sig_prev_mask, NULL)) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_SIGPROCMASK;
        error->syscall_errno = errno;
        return FAST_LOCK_ERROR;
    }
    if ((error->syscall_errno =
        pthread_mutexattr_destroy(&process_lock->mutex_attr))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_MUTEXATTR_DESTROY;
    	return FAST_LOCK_ERROR;
    }
    if ((error->syscall_errno =
        pthread_mutex_destroy(&process_lock->lock))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_MUTEX_DESTROY;
    	return FAST_LOCK_ERROR;
    }

    allocator = process_lock->allocator;
    if (allocator->free(allocator, process_lock, &error->allocator_errno)
        == FAST_MEM_ALLOCATOR_ERROR) {
        error->lock_errno = FAST_LOCK_ERR_ALLOCATOR_ERROR;
    }
    return FAST_LOCK_OK;
}
int fast_process_lock_reset(fast_process_lock_t *process_lock,
    fast_lock_errno_t *error)
{
	if (!error) {
	    return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_CLEAN(error);
	if (!process_lock) {
        error->lock_errno = FAST_LOCK_ERR_PARAM;
		return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_ALLOCATOR(error, process_lock->allocator);
    if ((error->syscall_errno = pthread_mutex_init(&process_lock->lock, 
        &process_lock->mutex_attr))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_MUTEX_INIT;
        return FAST_LOCK_ERROR;
    }

    return FAST_LOCK_OK;
}
int fast_process_lock_on(fast_process_lock_t *process_lock,
    fast_lock_errno_t *error)
{
    if (!error) {
        return FAST_LOCK_ERROR;
    }
    
    FAST_LOCK_ERRNO_CLEAN(error);
	if (!process_lock) {
        error->lock_errno = FAST_LOCK_ERR_PARAM;
        return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_ALLOCATOR(error, process_lock->allocator);

    //block signal except
    if (sigprocmask(SIG_BLOCK, &process_lock->sig_block_mask,
    	&process_lock->sig_prev_mask)) {
    	error->lock_errno = FAST_LOCK_ERR_SYSCALL_SIGPROCMASK;   
    	error->syscall_errno = errno;
    	return FAST_LOCK_ERROR;
    }
    
    if ((error->syscall_errno =
        pthread_mutex_lock(&process_lock->lock))) {
    	error->lock_errno = FAST_LOCK_ERR_SYSCALL_MUTEX_LOCK;   
    	return FAST_LOCK_ERROR;
    }
    
    return FAST_LOCK_ON;
}

int fast_process_lock_try_on(fast_process_lock_t *process_lock, 
    fast_lock_errno_t *error)
{
    if(!error) {
        return FAST_LOCK_ERROR;
    }

    FAST_LOCK_ERRNO_CLEAN(error);
	if (!process_lock) {
	    error->lock_errno = FAST_LOCK_ERR_PARAM; 
        return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_ALLOCATOR(error, process_lock->allocator);
    //block signal except
    if (sigprocmask(SIG_BLOCK, &process_lock->sig_block_mask,
        &process_lock->sig_prev_mask)) {
    	error->lock_errno = FAST_LOCK_ERR_SYSCALL_SIGPROCMASK;   
    	error->syscall_errno = errno;
        return FAST_LOCK_ERROR;
    }
    
    if ((error->syscall_errno =
        pthread_mutex_trylock(&process_lock->lock))) {
    	error->lock_errno = FAST_LOCK_ERR_SYSCALL_MUTEX_TRYLOCK;   
        return FAST_LOCK_OFF;
    }
    
 
    return FAST_LOCK_ON;

}

int fast_process_lock_off(fast_process_lock_t *process_lock,
    fast_lock_errno_t *error)
{
    if (!error) {
        return FAST_LOCK_ERROR;
    }
    
    FAST_LOCK_ERRNO_CLEAN(error);
	if (!process_lock) {
	    error->lock_errno = FAST_LOCK_ERR_PARAM;
        return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_ALLOCATOR(error, process_lock->allocator);
    if ((error->syscall_errno =
        pthread_mutex_unlock(&process_lock->lock))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_MUTEX_UNLOCK;
    	return FAST_LOCK_ERROR;
    }

    //recover signal
    if (sigprocmask(SIG_SETMASK, &process_lock->sig_prev_mask, NULL)) {
    	error->lock_errno = FAST_LOCK_ERR_SYSCALL_SIGPROCMASK;   
    	error->syscall_errno = errno;
    	return FAST_LOCK_ERROR;
    }
    
   
    return FAST_LOCK_OFF;
}

fast_atomic_lock_t *
fast_atomic_lock_create(fast_mem_allocator_t *allocator,
    fast_lock_errno_t *error)
{
    fast_atomic_lock_t *atomic_lock = NULL; 	
    
    if (!error) {
        return NULL;
    }
    FAST_LOCK_ERRNO_CLEAN(error);
    FAST_LOCK_ERRNO_ALLOCATOR(error, allocator);
    if (!allocator) {
        error->lock_errno = FAST_LOCK_ERR_PARAM;
        return NULL;
    }
    
    atomic_lock = allocator->alloc(allocator, sizeof(fast_atomic_lock_t),
        &error->allocator_errno);
	if (!atomic_lock) {
	    error->lock_errno = FAST_LOCK_ERR_ALLOCATOR_ERROR;
		return NULL;
    }
    atomic_lock->allocator = allocator; 
    atomic_lock->lock=FAST_LOCK_OFF; 
    
    return atomic_lock;
}
int fast_atomic_lock_init(fast_atomic_lock_t *atomic_lock)
{
    atomic_lock->lock = FAST_LOCK_OFF; 
    
    return FAST_LOCK_OK;
}
int fast_atomic_lock_release(fast_atomic_lock_t *atomic_lock,
    fast_lock_errno_t *error)
{
    fast_mem_allocator_t *allocator;
    if (!error) {
        return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_CLEAN(error);
	if (!atomic_lock || !atomic_lock->allocator) {
	    error->lock_errno = FAST_LOCK_ERR_PARAM;
		return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_ALLOCATOR(error, atomic_lock->allocator);
    allocator = atomic_lock->allocator; 
    if (allocator->free(allocator, atomic_lock, &error->allocator_errno)
        == FAST_MEM_ALLOCATOR_ERROR) {
        error->lock_errno = FAST_LOCK_ERR_ALLOCATOR_ERROR;
        return FAST_LOCK_ERROR;
    }
    
    return FAST_LOCK_OK;
}
int fast_atomic_lock_reset(fast_atomic_lock_t *atomic_lock,
    fast_lock_errno_t *error) 
{
    if (!error) {
        return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_CLEAN(error);
	if (!atomic_lock) {
	    error->lock_errno = FAST_LOCK_ERR_PARAM;
		return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_ALLOCATOR(error, atomic_lock->allocator);
    atomic_lock->lock = FAST_LOCK_OFF;
    return FAST_LOCK_OK;
}
int fast_atomic_lock_try_on(fast_atomic_lock_t *atomic_lock,
    fast_lock_errno_t *error)
{
    if (!error) {
        return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_CLEAN(error);
	if (!atomic_lock) {
        error->lock_errno = FAST_LOCK_ERR_PARAM;
		return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_ALLOCATOR(error, atomic_lock->allocator);

    return atomic_lock->lock == FAST_LOCK_OFF
        && CAS(&atomic_lock->lock, FAST_LOCK_OFF, FAST_LOCK_ON) ?
        FAST_LOCK_ON : FAST_LOCK_OFF;
}
int fast_atomic_lock_on(fast_atomic_lock_t *atomic_lock,
    fast_lock_errno_t *error)
{
    if (!error) {
        return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_CLEAN(error);
	if (!atomic_lock) {
	    error->lock_errno = FAST_LOCK_ERR_PARAM;
		return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_ALLOCATOR(error, atomic_lock->allocator);
    
    while(!CAS(&atomic_lock->lock, FAST_LOCK_OFF, FAST_LOCK_ON));
    
    return FAST_LOCK_ON;
}
int fast_atomic_lock_off(fast_atomic_lock_t *atomic_lock,
    fast_lock_errno_t *error)
{
    if (!error) {
        return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_CLEAN(error);
	if (!atomic_lock) {
        error->lock_errno = FAST_LOCK_ERR_PARAM;
		return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_ALLOCATOR(error, atomic_lock->allocator);
    
    if (atomic_lock->lock == FAST_LOCK_OFF) {
    	return FAST_LOCK_OFF;
    }
    
    while (!CAS(&atomic_lock->lock, FAST_LOCK_ON, FAST_LOCK_OFF));
    
    return FAST_LOCK_OFF;
}
fast_process_rwlock_t* fast_process_rwlock_create(fast_mem_allocator_t *allocator,
    fast_lock_errno_t *error)
{
    fast_process_rwlock_t   *process_rwlock = NULL;

    if (!error) {
        return NULL;
    }

    FAST_LOCK_ERRNO_CLEAN(error);
    FAST_LOCK_ERRNO_ALLOCATOR(error, allocator);

    if (!allocator) {
        error->lock_errno = FAST_LOCK_ERR_PARAM;
        return NULL;
    }
    process_rwlock = allocator->alloc(allocator, sizeof(fast_process_rwlock_t),
        &error->allocator_errno);
    if (!process_rwlock) {
        error->lock_errno = FAST_LOCK_ERR_ALLOCATOR_ERROR;
        return NULL;
    }
    //init rwlock attribute
    if ((error->syscall_errno =
        pthread_rwlockattr_init(&process_rwlock->rwlock_attr))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCKATTR_INIT;
        return NULL;
    }

    //set mutex attribute to process shared
    if ((error->syscall_errno =
        pthread_rwlockattr_setpshared(&process_rwlock->rwlock_attr,
    	PTHREAD_PROCESS_SHARED))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCKATTR_SETPSHARED;
        return NULL;
    }

    if ((error->syscall_errno = pthread_rwlock_init(&process_rwlock->rwlock,
    	&process_rwlock->rwlock_attr))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCK_INIT;
        return NULL;
    }
    
    process_rwlock->allocator = allocator;

    //remove SIGHUP SIGTERM SIGUSR1 from sigset of currnet process
    sigfillset(&process_rwlock->sig_block_mask);
    sigdelset(&process_rwlock->sig_block_mask, SIGALRM);
    sigdelset(&process_rwlock->sig_block_mask, SIGINT);
    sigdelset(&process_rwlock->sig_block_mask, SIGCHLD);
    sigdelset(&process_rwlock->sig_block_mask, SIGPIPE);
    sigdelset(&process_rwlock->sig_block_mask, SIGSEGV);
    sigdelset(&process_rwlock->sig_block_mask, SIGHUP);
    sigdelset(&process_rwlock->sig_block_mask, SIGQUIT);
    sigdelset(&process_rwlock->sig_block_mask, SIGTERM);
    sigdelset(&process_rwlock->sig_block_mask, SIGIO);
    sigdelset(&process_rwlock->sig_block_mask, SIGUSR1);

    return process_rwlock;    
}

int fast_process_rwlock_release(fast_process_rwlock_t* process_rwlock,
    fast_lock_errno_t *error)
{
    fast_mem_allocator_t *allocator;
    if (!error) {
        return FAST_LOCK_ERROR;
    }
    
    FAST_LOCK_ERRNO_CLEAN(error);
	if (!process_rwlock || !process_rwlock->allocator) {
        error->lock_errno = FAST_LOCK_ERR_PARAM;
		return FAST_LOCK_ERROR;
	}
    FAST_LOCK_ERRNO_ALLOCATOR(error, process_rwlock->allocator);
	//recover signal mask
    if (sigprocmask(SIG_SETMASK, &process_rwlock->sig_prev_mask, NULL)) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_SIGPROCMASK;
        error->syscall_errno = errno;
        return FAST_LOCK_ERROR;
    }
    
    if ((error->syscall_errno =
        pthread_rwlockattr_destroy(&process_rwlock->rwlock_attr))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCKATTR_DESTROY;
        return FAST_LOCK_ERROR;
    }
    
    if ((error->syscall_errno =
         pthread_rwlock_destroy(&process_rwlock->rwlock))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCK_DESTROY;
    	return FAST_LOCK_ERROR;
    }
    allocator = process_rwlock->allocator; 
    if (allocator->free(allocator, process_rwlock, &error->allocator_errno)
        == FAST_MEM_ALLOCATOR_ERROR) {
        error->lock_errno = FAST_LOCK_ERR_ALLOCATOR_ERROR;
        return FAST_LOCK_ERROR;
    }
    
    return FAST_LOCK_OK;
}

int fast_process_rwlock_reset(fast_process_rwlock_t *process_rwlock,
    fast_lock_errno_t *error)
{
    if (!error) {
        return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_CLEAN(error);
	if (!process_rwlock) {
        error->lock_errno = FAST_LOCK_ERR_PARAM;
		return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_ALLOCATOR(error, process_rwlock->allocator);

    if ((error->syscall_errno = pthread_rwlock_init(&process_rwlock->rwlock,
    	&process_rwlock->rwlock_attr))) {
    	error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCK_INIT;
        return FAST_LOCK_ERROR;
    }
    return FAST_LOCK_OK;
}
int fast_process_rwlock_read_on(fast_process_rwlock_t *process_rwlock,
    fast_lock_errno_t *error)
{
    
    if (!error) {
        return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_CLEAN(error);
	if (!process_rwlock) {
        error->lock_errno = FAST_LOCK_ERR_PARAM ;
        return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_ALLOCATOR(error, process_rwlock->allocator);

    //block signal except
    if (sigprocmask(SIG_BLOCK, &process_rwlock->sig_block_mask,
    	&process_rwlock->sig_prev_mask)) {
    	error->lock_errno = FAST_LOCK_ERR_SYSCALL_SIGPROCMASK;
    	error->syscall_errno = errno;
    	return FAST_LOCK_ERROR;
    }
    if ((error->syscall_errno =
        pthread_rwlock_rdlock(&process_rwlock->rwlock))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCK_RDLOCK;
    	return FAST_LOCK_ERROR;
    }
    return FAST_LOCK_ON;
}

int fast_process_rwlock_off(fast_process_rwlock_t *process_rwlock,
    fast_lock_errno_t *error)
{
    if (!error) {
        return FAST_LOCK_ERROR;
    }
    
    FAST_LOCK_ERRNO_CLEAN(error);
	if (!process_rwlock) {
        error->lock_errno = FAST_LOCK_ERR_PARAM ;
        return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_ALLOCATOR(error, process_rwlock->allocator);
    
    if ((error->syscall_errno = pthread_rwlock_unlock(&process_rwlock->rwlock))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCK_UNLOCK;
    	return FAST_LOCK_ERROR;
    }

    //recover signal
    if (sigprocmask(SIG_SETMASK, &process_rwlock->sig_prev_mask, NULL)) {
    	error->lock_errno = FAST_LOCK_ERR_SYSCALL_SIGPROCMASK;
    	error->syscall_errno = errno;
    	return FAST_LOCK_ERROR;
    }
    
    return FAST_LOCK_OFF;
}
int fast_process_rwlock_write_on(fast_process_rwlock_t *process_rwlock,
    fast_lock_errno_t *error)
{
    if (!error) {
        return FAST_LOCK_ERROR;
    }
    
    FAST_LOCK_ERRNO_CLEAN(error);
    error->lock_errno = FAST_LOCK_ERR_NONE;
	if (!process_rwlock) {
        error->lock_errno = FAST_LOCK_ERR_PARAM ;
        return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_ALLOCATOR(error, process_rwlock->allocator);

    //block signal except
    if (sigprocmask(SIG_BLOCK, &process_rwlock->sig_block_mask,
    	&process_rwlock->sig_prev_mask)) {
    	error->lock_errno = FAST_LOCK_ERR_SYSCALL_SIGPROCMASK;
    	error->syscall_errno = errno;
    	return FAST_LOCK_ERROR;
    }
    
    if ((error->syscall_errno =
        pthread_rwlock_wrlock(&process_rwlock->rwlock))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCK_WRLOCK;
    	return FAST_LOCK_ERROR;
    }
    
    return FAST_LOCK_ON;
}
int fast_process_rwlock_write_try_on(fast_process_rwlock_t *process_rwlock,
    fast_lock_errno_t *error)
{
    if (!error) {
        return FAST_LOCK_ERROR;
    }
    
    FAST_LOCK_ERRNO_CLEAN(error);
	if (!process_rwlock) {
        error->lock_errno = FAST_LOCK_ERR_PARAM ;
        return FAST_LOCK_ERROR;
    }
    FAST_LOCK_ERRNO_ALLOCATOR(error, process_rwlock->allocator);
    if (sigprocmask(SIG_BLOCK, &process_rwlock->sig_block_mask,
    	&process_rwlock->sig_prev_mask)) {
    	error->lock_errno = FAST_LOCK_ERR_SYSCALL_SIGPROCMASK;
    	error->syscall_errno = errno;
    	return FAST_LOCK_ERROR;
    }
    
    if ((error->syscall_errno =
        pthread_rwlock_trywrlock(&process_rwlock->rwlock))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCK_TRYWRLOCK;
    	return FAST_LOCK_ERROR;
    }
    
    return FAST_LOCK_ON;
}

const char*
fast_lock_strerror(fast_lock_errno_t *error)
{
    if (!error) {
        return fast_lock_err_string[0];
    }

    if (error->lock_errno <= FAST_LOCK_ERR_START
        || error->lock_errno >= FAST_LOCK_ERR_END) {
        return fast_lock_err_string[0];
    }

    if (error->lock_errno == FAST_LOCK_ERR_ALLOCATOR_ERROR
        && error->err_allocator
        && error->err_allocator->strerror) {
        return error->err_allocator->strerror(error->err_allocator,
            &error->allocator_errno);
    }
    if (error->lock_errno >= FAST_LOCK_ERR_SYSCALL_MUTEXATTR_INIT
        && error->lock_errno <= FAST_LOCK_ERR_SYSCALL_RWLOCK_TRYWRLOCK) {
        return strerror(error->syscall_errno);
    }

    return fast_lock_err_string[error->lock_errno - FAST_LOCK_ERR_START];
}


inline void fast_atomic_lock_off_force(fast_atomic_lock_t *atomic_lock)
{
    atomic_lock->lock = FAST_LOCK_OFF;
}

fast_atomic_lock_t *fast_atomic_lock_pool_create(pool_t *pool)
{
    fast_atomic_lock_t *atomic_lock = NULL; 	
    if (!pool) {
        return NULL;
    }
    atomic_lock = pool_alloc(pool, sizeof(fast_atomic_lock_t));
	if (!atomic_lock) {
		return NULL;
    }
    atomic_lock->lock = FAST_LOCK_OFF; 
    return atomic_lock;
}
int fast_process_rwlock_init(fast_process_rwlock_t * process_rwlock,
    fast_lock_errno_t *error)
{
    if (!error) {
        return FAST_LOCK_ERROR;
    }

    //init rwlock attribute
    if ((error->syscall_errno =
        pthread_rwlockattr_init(&process_rwlock->rwlock_attr))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCKATTR_INIT;
        return FAST_LOCK_ERROR;
    }

    //set mutex attribute to process shared
    if ((error->syscall_errno =
        pthread_rwlockattr_setpshared(&process_rwlock->rwlock_attr,
    	PTHREAD_PROCESS_SHARED))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCKATTR_SETPSHARED;
        return FAST_LOCK_ERROR;
    }

    if ((error->syscall_errno = pthread_rwlock_init(&process_rwlock->rwlock,
    	&process_rwlock->rwlock_attr))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCK_INIT;
        return FAST_LOCK_ERROR;
    }
    
    //remove SIGHUP SIGTERM SIGUSR1 from sigset of currnet process
    sigfillset(&process_rwlock->sig_block_mask);
    sigdelset(&process_rwlock->sig_block_mask, SIGALRM);
    sigdelset(&process_rwlock->sig_block_mask, SIGINT);
    sigdelset(&process_rwlock->sig_block_mask, SIGCHLD);
    sigdelset(&process_rwlock->sig_block_mask, SIGPIPE);
    sigdelset(&process_rwlock->sig_block_mask, SIGSEGV);
    sigdelset(&process_rwlock->sig_block_mask, SIGHUP);
    sigdelset(&process_rwlock->sig_block_mask, SIGQUIT);
    sigdelset(&process_rwlock->sig_block_mask, SIGTERM);
    sigdelset(&process_rwlock->sig_block_mask, SIGIO);
    sigdelset(&process_rwlock->sig_block_mask, SIGUSR1);

    return FAST_LOCK_OK;    
}
int fast_process_rwlock_destroy(fast_process_rwlock_t* process_rwlock,
    fast_lock_errno_t *error)
{
    
    if (sigprocmask(SIG_SETMASK, &process_rwlock->sig_prev_mask, NULL)) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_SIGPROCMASK;
        error->syscall_errno = errno;
        return FAST_LOCK_ERROR;
    }
    
    if ((error->syscall_errno =
        pthread_rwlockattr_destroy(&process_rwlock->rwlock_attr))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCKATTR_DESTROY;
        return FAST_LOCK_ERROR;
    }
    
    if ((error->syscall_errno =
         pthread_rwlock_destroy(&process_rwlock->rwlock))) {
        error->lock_errno = FAST_LOCK_ERR_SYSCALL_RWLOCK_DESTROY;
    	return FAST_LOCK_ERROR;
    }
      
    return FAST_LOCK_OK;
}