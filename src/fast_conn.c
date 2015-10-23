#include "fast_conn.h"
#include "fast_sysio.h"
#include <netinet/in.h>
#include "fast_event.h"
#include "fast_epoll.h"
#include "fast_memory.h"
#include "fast_event_timer.h"
#include "fast_epoll.h"

int
conn_connect_peer(conn_peer_t *pc, event_base_t *ep_base)
{
    int            rc = 0;
    //int            tcp_maxseg = 4096;
    conn_t        *c = NULL;
    event_t       *wev = NULL;

    if (!pc) {
        return FAST_ERROR;
    }
    //configure peer connection
    

    c = pc->connection;
    if (!c) {
        return FAST_BUSY;
    }
    
    if (c->fd != FAST_INVALID_FILE) {
        goto connecting;
    }
    
    c->fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (c->fd == FAST_ERROR) {
        // add log
        return FAST_ERROR;
    }
    
    conn_nonblocking(c->fd);
   
    c->recv = fast_recv;
    c->send = fast_send;
    c->recv_chain = fast_recv_chain;
    c->send_chain = fast_send_chain;
    c->sendfile_chain = fast_sendfile_chain;
    c->sendfile = FAST_TRUE;
    if (pc->sockaddr->sa_family != AF_INET) {
        c->tcp_nopush = CONN_TCP_NOPUSH_DISABLED;
        c->tcp_nodelay = CONN_TCP_NODELAY_DISABLED;
    }
    c->tcp_nodelay = CONN_TCP_NODELAY_UNSET;
    c->tcp_nopush = CONN_TCP_NOPUSH_UNSET;

    wev = c->write;
    
connecting:
    
    //add conn to epoll, read write
    if (event_add_conn(ep_base, c) == FAST_ERROR) {
        return FAST_ERROR;
    }
    
    errno = 0;
    rc = connect(c->fd, pc->sockaddr, pc->socklen);
    if (rc == FAST_ERROR) {
        if (errno == FAST_EINPROGRESS) {
            return FAST_AGAIN;
        }
        return FAST_ERROR;
    }

    wev->ready = 1;

    return FAST_OK;
}

int
conn_tcp_nopush(int s)
{
    int  cork = 1;
    return setsockopt(s, IPPROTO_TCP, TCP_CORK,
        (const void *) &cork, sizeof(int));
}

int
conn_tcp_push(int s)
{
    int  cork = 0;
    return setsockopt(s, IPPROTO_TCP, TCP_CORK,
        (const void *) &cork, sizeof(int));
}

int
conn_tcp_nodelay(int s)
{
    int  nodelay = 1;
    return setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
        (const void *) &nodelay, sizeof(int));
}

int
conn_tcp_delay(int s)
{
    int  nodelay = 0;
    return setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
        (const void *) &nodelay, sizeof(int));
}
conn_t *conn_get_from_mem(int s)
{
    conn_t       *c   = NULL;
    event_t      *rev = NULL;
    event_t      *wev = NULL;

    c   = memory_calloc(sizeof(conn_t));
    rev = memory_calloc(sizeof(event_t));
    wev = memory_calloc(sizeof(event_t));

    if (!c || !rev || !wev) {
        return NULL;
    }
    c->read = rev;
    c->write = wev;
    conn_set_default(c, s);
    return c;
}

void conn_set_default(conn_t *c, int s)
{
    event_t      *rev = NULL;
    event_t      *wev = NULL;
    uint32_t      instance;
    uint32_t      last_instance;

    c->fd = s;

    rev = c->read;
    wev = c->write;
    instance = rev->instance;
    last_instance = rev->last_instance;
    c->sent = 0;

    c->conn_data = NULL;
    c->next = NULL;
    c->error = 0;
    c->listening = NULL;
    c->next = NULL;
    c->sendfile = FAST_FALSE;
    c->sndlowat = 0;
    c->sockaddr = NULL;
    memory_zero(&c->addr_text, sizeof(string_t));
    c->socklen = 0;
    //set rev & wev->instance to !last->instance
    memory_zero(rev, sizeof(event_t));
    memory_zero(wev, sizeof(event_t));
    rev->instance = !instance;
    wev->instance = !instance;
    rev->last_instance = last_instance;
    
    rev->data = c;
    wev->data = c;
    wev->write = FAST_TRUE;

}
void conn_close(conn_t *c)
{
    if (!c) {
        return;
    }
    if (c->fd > 0) {
        close(c->fd);
        c->fd = FAST_INVALID_FILE;
    
        //remove timers
        if (c->read->timer_set && c->ev_timer) {
            event_timer_del(c->ev_timer, c->read);
        }

        if (c->write->timer_set && c->ev_timer) {
            event_timer_del(c->ev_timer, c->write);
        }

        if (c->ev_base) {
            event_del_conn(c->ev_base, c, EVENT_CLOSE_EVENT);
        }

    } 
}

void conn_release(conn_t *c)
{
    conn_close(c);

    if (c->pool) {
        pool_destroy(c->pool);
    }
    c->pool = NULL;
}
void conn_free_mem(conn_t *c)
{
    event_t      *rev = NULL;
    event_t      *wev = NULL;

    rev = c->read;
    wev = c->write;
    memory_free(rev,sizeof(event_t));
    memory_free(wev,sizeof(event_t));
    memory_free(c, sizeof(conn_t));
}

