#include "fast_conn_pool.h"
#include "fast_memory.h"
#include "fast_event_timer.h"
#include "fast_lock.h"
#include "fast_conn.h"

conn_pool_t comm_conn_pool;
fast_atomic_lock_t comm_conn_lock;

static void put_comm_conn(conn_t *c);
static conn_t* get_comm_conn(uint32_t n, int *num);

int conn_pool_init(conn_pool_t *pool, uint32_t connection_n)
{
    conn_t        *conn = NULL;
    event_t       *revs = NULL;
    event_t       *wevs = NULL;
    uint32_t       i = 0;

    if (connection_n == 0) {
        return FAST_ERROR;
    }

    pool->connection_n = connection_n;
    pool->connections = memory_calloc(sizeof(conn_t) * pool->connection_n);
    if (!pool->connections) {
        return FAST_ERROR;
    }

    pool->read_events = memory_calloc(sizeof(event_t) * pool->connection_n);
    if (!pool->read_events) {
        conn_pool_free(pool);
        return FAST_ERROR;
    }

    pool->write_events = memory_calloc(sizeof(event_t) * pool->connection_n);
    if (!pool->write_events) {
        conn_pool_free(pool);
        return FAST_ERROR;
    }

    conn = pool->connections;
    revs = pool->read_events;
    wevs = pool->write_events;

    for (i = 0; i < pool->connection_n; i++) {
        revs[i].instance = 1;

        if (i == pool->connection_n - 1) {
            conn[i].next = NULL;
        } else {
            conn[i].next = &conn[i + 1];
        }

        conn[i].fd = FAST_INVALID_FILE;
        conn[i].read = &revs[i];
        conn[i].read->timer_event = FAST_FALSE;
        conn[i].write = &wevs[i];
        conn[i].write->timer_event = FAST_FALSE;
    }

    pool->free_connections = conn;
    pool->free_connection_n = pool->connection_n;

    return FAST_OK;
}

void
conn_pool_free(conn_pool_t *pool)
{
    if (!pool) {
        return;
    }

    if (pool->connections) {
        memory_free(pool->connections,
            sizeof(conn_t) * pool->connection_n);
        pool->connections = NULL;
    }

    if (pool->read_events) {
        memory_free(pool->read_events, sizeof(event_t) * pool->connection_n);
        pool->read_events = NULL;
    }

    if (pool->write_events) {
        memory_free(pool->write_events, sizeof(event_t) * pool->connection_n);
        pool->write_events = NULL;
    }

    pool->connection_n = 0;
    pool->free_connection_n = 0;
}

conn_t *
conn_pool_get_connection(conn_pool_t *pool)
{
    conn_t       *c    = NULL;
    int           num  = 0;
	uint32_t      n    = 0;

    c = pool->free_connections;
    if (!c) {
        if (pool->change_n >= 0) {
            return NULL;
        }
		n = -pool->change_n;
        c = get_comm_conn(n, &num);
        if (c) {
            pool->free_connections = c;
            pool->free_connection_n += num;
			pool->change_n += num;
            goto out_conn;
        }

        return NULL;
    }
    //set free_connections to this conn's next
out_conn:
    pool->free_connections = c->next;
    pool->free_connection_n--;
	pool->used_n++;
    return c;
}

void conn_pool_free_connection(conn_pool_t *pool, conn_t *c)
{   
    if (pool->change_n > 0) {
        pool->used_n--;
        put_comm_conn(c);
		pool->change_n--;
        return;
    }
    c->next = pool->free_connections;

    pool->free_connections = c;
    pool->free_connection_n++;
	pool->used_n--;

}

int conn_pool_common_init()
{
    comm_conn_lock.lock = FAST_LOCK_OFF;
    comm_conn_lock.allocator = NULL;
    memset(&comm_conn_pool, 0, sizeof(conn_pool_t));
    return FAST_OK;
}
int conn_pool_common_release()
{
    comm_conn_lock.lock = FAST_LOCK_OFF;
    comm_conn_lock.allocator = NULL;
    return FAST_OK;
}

static void put_comm_conn(conn_t *c)
{
	fast_lock_errno_t error;
   
    fast_atomic_lock_on(&comm_conn_lock, &error);
    comm_conn_pool.free_connection_n++;
    c->next = comm_conn_pool.free_connections;
    comm_conn_pool.free_connections = c;
    
    fast_atomic_lock_off(&comm_conn_lock, &error);
}
static conn_t* get_comm_conn(uint32_t n, int *num)
{
	fast_lock_errno_t error;
    conn_t *c, *p, *plast;
    uint32_t i;
		
	fast_atomic_lock_on(&comm_conn_lock, &error);
	
    if (!comm_conn_pool.free_connection_n) {
        fast_atomic_lock_off(&comm_conn_lock, &error);
        return NULL;
    }

    if (n >= comm_conn_pool.free_connection_n) {

        c = comm_conn_pool.free_connections;
        *num = comm_conn_pool.free_connection_n;

        comm_conn_pool.free_connections = NULL;
        comm_conn_pool.free_connection_n = 0;

        fast_atomic_lock_off(&comm_conn_lock, &error);
        return c;
    }
	
	p = comm_conn_pool.free_connections	;
    i = n;
	for (plast = p; p && i > 0; i--) {
		plast = p;
		p = p->next;
	}
	
	c = comm_conn_pool.free_connections;
    
	comm_conn_pool.free_connections = p;
    comm_conn_pool.free_connection_n -= n;
    
	plast->next = NULL;
    *num = n;
    
    fast_atomic_lock_off(&comm_conn_lock, &error);

    return c;
}

void conn_pool_out(conn_pool_t *pool, int n) 
{
	pool->change_n -= n; 
	pool->used_n -=n; 
}
		
void conn_pool_in(conn_pool_t *pool, int n) 
{
	pool->change_n += n; 
	pool->used_n += n; 
}

