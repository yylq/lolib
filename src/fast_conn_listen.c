#include "fast_conn_listen.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "fast_time.h"
#include "fast_memory.h"
#include "fast_event.h"
#include "fast_epoll.h"

#define FAST_INET_ADDRSTRLEN (sizeof("255.255.255.255") - 1)

int conn_listening_open(array_t *listening, log_t *log)
{
    int            s = FAST_INVALID_FILE;
    int            reuseaddr = 1;
    uint32_t       i = 0;
    uint32_t       tries = 0;
    uint32_t       failed = 0;
    listening_t   *ls = NULL;

    if (!listening || !log) {
        return FAST_ERROR;
    }

    /* TODO: configurable try number */
    for (tries = 5; tries; tries--) {
        failed = 0;
        //for each listening socket
        ls = listening->elts;
        for (i = 0; i < listening->nelts; i++) {
            if (ls[i].ignore) {
                continue;
            }

            if (ls[i].fd != FAST_INVALID_FILE) {
                fast_log_error(log, FAST_LOG_ALERT, 0,
                    "conn_listening_open: %V, fd:%d already opened",
                    &ls[i].addr_text, ls[i].fd);
                continue;
            }

            if (ls[i].inherited) {
                /* TODO: deferred accept */
                continue;
            }

            s = socket(ls[i].family, ls[i].type, 0);
            if (s == FAST_INVALID_FILE) {
                fast_log_error(log, FAST_LOG_ERROR, errno,
                    "conn_listening_open: create socket on %V failed",
                    &ls[i].addr_text);
                return FAST_ERROR;
            }

            if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                (const void *) &reuseaddr, sizeof(int)) == FAST_ERROR) {
                fast_log_error(log, FAST_LOG_ERROR, errno,
                    "conn_listening_open: SO_REUSEADDR %V failed",
                    &ls[i].addr_text);
                goto error;
            }

            if (ls[i].rcvbuf != -1) {
                if (setsockopt(s, SOL_SOCKET, SO_RCVBUF,
                    (const void *) &ls[i].rcvbuf, sizeof(int)) == FAST_ERROR) {
                    fast_log_error(log, FAST_LOG_ALERT, 0,
                        "conn_listening_open: SO_RCVBUF fd:%d "
                        "rcvbuf:%d addr:%V failed, ignored",
                        s, ls[i].rcvbuf, &ls[i].addr_text);
                }

            }

            if (ls[i].sndbuf != -1) {
                if (setsockopt(s, SOL_SOCKET, SO_SNDBUF,
                    (const void *) &ls[i].sndbuf, sizeof(int)) == FAST_ERROR) {
                    fast_log_error(log, FAST_LOG_ALERT, 0,
                        "conn_listening_open: SO_SNDBUF fd:%d "
                        "rcvbuf:%d addr:%V failed, ignored",
                        s, ls[i].sndbuf, &ls[i].addr_text);
                }

            }

            //we can't set linger onoff = 1 on listening socket
            if (conn_nonblocking(s) == FAST_ERROR) {
                fast_log_error(log, FAST_LOG_EMERG, errno,
                    "conn_listening_open: noblocking fd:%d "
                    "addr:%V failed", &ls[i].addr_text);
                goto error;
            }

            fast_log_debug(log, FAST_LOG_DEBUG, 0,
                "conn_listening_open: bind fd:%d on addr:%V",
                s, &ls[i].addr_text);
            if (bind(s, ls[i].sockaddr, ls[i].socklen) == FAST_ERROR) {
                fast_log_error(log, FAST_LOG_EMERG, errno,
                    "conn_listening_open: bind fd:%d on addr:%V failed",
                    s, &ls[i].addr_text);
                close(s);
                if (errno != FAST_EADDRINUSE) {
                    return FAST_ERROR;
                }

                failed = 1;
                continue;
            }

            if (listen(s, ls[i].backlog) == FAST_ERROR) {
                fast_log_error(log, FAST_LOG_EMERG, errno,
                    "conn_listening_open: listen fd:%d on addr:%V, "
                    "backlog:%d failed", s, &ls[i].addr_text, ls[i].backlog);
                goto error;
            }

            ls[i].listen = 1;
            ls[i].open = 1;
            ls[i].fd = s;
            ls[i].log = log;
            fast_log_debug(log, FAST_LOG_DEBUG, 0,
                "ls[%d] %V ,fd %d", i, &ls[i].addr_text, s);
        }

        if (!failed) {
            break;
        }

        /* TODO: delay configurable */
        fast_log_error(log, FAST_LOG_NOTICE, 0,
            "conn_listening_open: bind failed, try again after 500ms");
        time_msleep(500);
    }

    if (failed) {
        fast_log_error(log, FAST_LOG_EMERG, 0,
            "conn_listening_open: listening socket bind failed");
        return FAST_ERROR;
    }

    return FAST_OK;
error:
    close(s);
    return FAST_ERROR;
}

listening_t * conn_listening_add(array_t *listening, pool_t *pool, 
    log_t *log, in_addr_t addr, in_port_t port, event_handler_pt handler,
    int rbuff_len, int sbuff_len)
{
    uchar_t             *address = NULL;
    listening_t        *ls = NULL;
    struct sockaddr_in *sin = NULL;

    if (!listening || !pool || !log ||  (port <= 0)) {
        return NULL;
    }
    //alloc sin addr
    sin = pool_alloc(pool, sizeof(struct sockaddr_in));
    if (!sin) {
        fast_log_error(log, FAST_LOG_ALERT, 0,
            "conn_listening_add: pooll alloc sockaddr failed");
        return NULL;
    }
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = addr;
    sin->sin_port = htons(port);
    address = (uchar_t *)inet_ntoa(sin->sin_addr);
    //push listening socket
    ls = array_push(listening);
    if (!ls) {
        fast_log_error(log, FAST_LOG_ALERT, 0,
            "conn_listening_add: push listening socket failed!");
        return NULL;
    }
    memory_zero(ls, sizeof(listening_t));
    ls->addr_text.data = pool_calloc(pool,
        INET_ADDRSTRLEN - 1 + sizeof(":65535") - 1);
    if (!ls->addr_text.data) {
        fast_log_error(log, FAST_LOG_ALERT, 0,
            "conn_listening_add: pool alloc ls->addr text failed");
        return NULL;
    }
    ls->addr_text.len = string_xxsprintf(ls->addr_text.data,
        "%s:%d", address, port) - ls->addr_text.data;
    ls->fd = FAST_INVALID_FILE;
    ls->family = AF_INET;
    ls->type = SOCK_STREAM;
    ls->sockaddr = (struct sockaddr *) sin;
    ls->socklen = sizeof(struct sockaddr_in);
    //config from config file
    ls->backlog = CONN_DEFAULT_BACKLOG;
   	ls->rcvbuf = rbuff_len > CONN_DEFAULT_RCVBUF? rbuff_len: CONN_DEFAULT_RCVBUF;
    ls->sndbuf = sbuff_len > CONN_DEFAULT_SNDBUF? sbuff_len: CONN_DEFAULT_SNDBUF;
	
    //connection pool size
    ls->conn_psize = CONN_DEFAULT_POOL_SIZE;
    ls->log = log;
    ls->handler = handler;
    ls->open = 0;
    ls->linger = 1;

    return ls;
}

int conn_listening_close(array_t *listening)
{
    size_t         i;
    listening_t   *ls = NULL;

    for (i = 0; i < listening->nelts; i++) {
        if (ls[i].fd != FAST_INVALID_FILE) {
            close(ls[i].fd );
        }
    }

    return FAST_OK;
}

int conn_listening_add_event(event_base_t *base, array_t *listening)
{
    conn_t        *c = NULL;
    event_t       *rev = NULL;
    uint32_t       i = 0;
    listening_t   *ls = NULL;
      
    ls = listening->elts;
    for (i = 0; i < listening->nelts; i++) {
        c = ls[i].connection;
        if (!c) {
            c = conn_get_from_mem(ls->fd);
            if (!c) {
                fast_log_debug(ls[i].log, FAST_LOG_DEBUG, 0,
                    "add listening %V,fd %d",
                    &ls[i].addr_text, ls[i].fd);
                return FAST_ERROR;
            }
            fast_log_debug(ls[i].log, FAST_LOG_DEBUG, 0,
                "add listening %V,fd %d", &ls[i].addr_text, ls[i].fd);
            //each server log?
            
            c->listening = &ls[i];
            c->log = ls[i].log;
            ls[i].connection = c;
            rev = c->read;
            rev->accepted = FAST_TRUE;
            rev->handler = ls->handler;
        } else {
            rev = c->read;
        }
        //setup listenting event
        if (event_add(base, rev, EVENT_READ_EVENT, 0) == FAST_ERROR) {
            return FAST_ERROR;
        }
    }

    return FAST_OK;
}

int conn_listening_del_event(event_base_t *base, array_t *listening)
{
    conn_t        *c = NULL;
    uint32_t       i = 0;
    listening_t   *ls = NULL;

    ls = listening->elts;
    for (i = 0; i < listening->nelts; i++) {
        c = ls[i].connection;
        if (event_delete(base, c->read, EVENT_READ_EVENT, 0) == FAST_ERROR) {
            return FAST_ERROR;
        }
    }

    return FAST_OK;
}


