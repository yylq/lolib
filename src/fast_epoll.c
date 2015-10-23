#include "fast_epoll.h"
#include "fast_memory.h"
#include "fast_error_log.h"
#include "fast_event.h"
#include "fast_conn.h"

int
epoll_init(event_base_t *ep_base, log_t *log)
{
    ep_base->ep = epoll_create(ep_base->nevents);
    if (ep_base->ep == FAST_INVALID_FILE) {
        fast_log_error(log, FAST_LOG_EMERG,
            errno, "epoll_init: epoll_create failed");
        return FAST_ERROR;
    }

    //alloc event list
    ep_base->event_list = memory_calloc(
        sizeof(struct epoll_event) * ep_base->nevents);
    if (!ep_base->event_list) {
        fast_log_error(log, FAST_LOG_EMERG, 0,
            "epoll_init: alloc event_list failed");
        return FAST_ERROR;
    }
    //set event_flags
#if (EVENT_HAVE_CLEAR_EVENT)
    ep_base->event_flags = EVENT_USE_CLEAR_EVENT
#else
    ep_base->event_flags = EVENT_USE_LEVEL_EVENT
#endif
        |EVENT_USE_GREEDY_EVENT
        |EVENT_USE_EPOLL_EVENT;
    queue_init(&ep_base->posted_accept_events);
    queue_init(&ep_base->posted_events);
    ep_base->log = log;
    return FAST_OK;
}

void
epoll_done(event_base_t *ep_base)
{
    if (close(ep_base->ep) == FAST_ERROR) {
        fast_log_error(ep_base->log, FAST_LOG_ALERT, errno,
            "epoll close() failed");
    }

    ep_base->ep = FAST_INVALID_FILE;
    if (ep_base->event_list) {
        memory_free(ep_base->event_list,
            sizeof(struct epoll_event) * ep_base->nevents);
        ep_base->event_list = NULL;
    }

    ep_base->nevents = 0;
    ep_base->event_flags = 0;
}

int
epoll_add_event(event_base_t *ep_base, event_t *ev, int event, uint32_t flags)
{
    int                  op;
    conn_t              *c;
    event_t             *aevent;
    uint32_t             events;
    uint32_t             aevents;
    struct epoll_event   ee;
    
    memory_zero(&ee, sizeof(ee));
    c = ev->data;
    events = (uint32_t) event;

    if (event == EVENT_READ_EVENT) {
        aevent = c->write;
        aevents = EPOLLOUT;
    } else {
        aevent = c->read;
        aevents = EPOLLIN;
    }
    //another event is active, don't not forget it
    if (aevent->active) {
        op = EPOLL_CTL_MOD;
        events |= aevents;
    } else {
        op = EPOLL_CTL_ADD;
    }
    //flag is EPOLLET
    ee.events = events | (uint32_t) flags;
    ee.data.ptr = (void *) ((uintptr_t) c | ev->instance);

    ev->active = FAST_TRUE;

    if (epoll_ctl(ep_base->ep, op, c->fd, &ee) == -1) {
        fast_log_error(ep_base->log, FAST_LOG_ALERT, errno,
            "epoll_add_event: fd:%d op:%d, failed", c->fd, op);
        ev->active = FAST_FALSE;
        return FAST_ERROR;
    }

    //ev->active = FAST_TRUE;

    return FAST_OK;
}

int
epoll_del_event(event_base_t *ep_base, event_t *ev, int event, uint32_t flags)
{
    int                  op;
    conn_t              *c;
    event_t             *e;
    uint32_t             prev;
    struct epoll_event   ee;
    
    memory_zero(&ee, sizeof(ee));

    c = ev->data;
 
    /*
     * when the file descriptor is closed, the epoll automatically deletes
     * it from its queue, so we do not need to delete explicity the event
     * before the closing the file descriptor
     */
    if (flags & EVENT_CLOSE_EVENT) {
        ev->active = 0;
        return FAST_OK;
    }

    if (event == EVENT_READ_EVENT) {
        e = c->write;
        prev = EPOLLOUT;
    } else {
        e = c->read;
        prev = EPOLLIN;
    }

    if (e->active) {
        op = EPOLL_CTL_MOD;
        ee.events = prev | (uint32_t) flags;
        ee.data.ptr = (void *) ((uintptr_t) c | ev->instance);
    } else {
        op = EPOLL_CTL_DEL;
        //ee.events = 0;
        ee.events = event;
        ee.data.ptr = NULL;
    }

    //fast_log_debug(FAST_LOG_DEBUG_EVENT, 0, "%s: c:%p, events:%ud, fd:%d",
    //        __func__, c, ee.events, c->fd);

    if (epoll_ctl(ep_base->ep, op, c->fd, &ee) == -1) {
        fast_log_error(ep_base->log, FAST_LOG_ALERT, errno,
            "epoll_ctl(%d, %d) failed", op, c->fd);
        return FAST_ERROR;
    }

    ev->active = 0;
    return FAST_OK;
}

int
epoll_add_connection(event_base_t *ep_base, conn_t *c)
{
    struct epoll_event ee;
    
    if (!c) {
        return FAST_ERROR;
    }

    memory_zero(&ee, sizeof(ee));
    ee.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ee.data.ptr = (void *) ((uintptr_t) c | c->read->instance);


    if (epoll_ctl(ep_base->ep, EPOLL_CTL_ADD, c->fd, &ee) == -1) {
        fast_log_error(ep_base->log, FAST_LOG_ALERT, errno,
            "epoll_ctl(EPOLL_CTL_ADD, %d) failed", c->fd);
        return FAST_ERROR;
    }

    c->read->active = FAST_TRUE;
    c->write->active = FAST_TRUE;

    return FAST_OK;
}

int
epoll_del_connection(event_base_t *ep_base, conn_t *c, uint32_t flags)
{
    int                op;
    struct epoll_event ee;
 
    if (!ep_base) {
        return FAST_OK;
    }
    /*
     * when the file descriptor is closed the epoll automatically deletes
     * it from its queue so we do not need to delete explicity the event
     * before the closing the file descriptor
     */
   
    if (flags & EVENT_CLOSE_EVENT) {
        c->read->active = 0;
        c->write->active = 0;
        return FAST_OK;
    }

    op = EPOLL_CTL_DEL;
    ee.events = 0;
    ee.data.ptr = NULL;
    if (epoll_ctl(ep_base->ep, op, c->fd, &ee) == -1) {
        fast_log_error(ep_base->log, FAST_LOG_ALERT, errno,
            "epoll_ctl(%d, %d) failed", op, c->fd);
        return FAST_ERROR;
    }

    c->read->active = 0;
    c->write->active = 0;

    return FAST_OK;
}

int
epoll_process_events(event_base_t *ep_base, rb_msec_t timer, uint32_t flags)
{
    int               i = 0;
    int               hd_num = 0;
    int               events_num = 0;
    uint32_t          events = 0;
    int               instance = 0;
    conn_t           *c = NULL;
    //conn_t           *ec = NULL;
    event_t          *rev= NULL;
    event_t          *wev = NULL;
    volatile queue_t *queue = NULL;
    
    //don't delete this comments, used for sometime debug
    //rb_msec_t         epwait_delta;

    errno = 0;
    //epwait_delta = event_get_curtime();
    events_num = epoll_wait(ep_base->ep, ep_base->event_list,
        (int) ep_base->nevents, timer);

    if (flags & EVENT_UPDATE_TIME && ep_base->time_update) {
        ep_base->time_update();
    }
    if (events_num == -1) {
        fast_log_error(ep_base->log, FAST_LOG_EMERG, errno,
            "epoll_process_events: epoll_wait failed");
        return FAST_ERROR;
    }
    if (events_num == 0) {
        if (timer != EVENT_TIMER_INFINITE) {
            return FAST_OK;
        }
        fast_log_error(ep_base->log, FAST_LOG_ALERT, errno,
            "epoll_process_events: epoll_wait no events or timeout");
        return FAST_ERROR;
    }

    for (i = 0; i < events_num; i++) {
        c = ep_base->event_list[i].data.ptr;
        instance = (uintptr_t) c & 1;
        c = (conn_t *) ((uintptr_t) c & (uintptr_t) ~1);

        rev = c->read;
        if (c->fd == FAST_INVALID_FILE || rev->instance != instance) {
            /*
             * the stale event from a file descriptor
             * that was just closed in this iteration
             */
            fast_log_debug(ep_base->log, FAST_LOG_DEBUG, 0,
                "epoll_process_events: stale event %p", c);
            continue;
        }

        events = ep_base->event_list[i].events;
        if (events & (EPOLLERR|EPOLLHUP)) {
            fast_log_debug(ep_base->log, FAST_LOG_DEBUG, errno,
                "epoll_process_events: epoll_wait error on fd:%d ev:%ud",
                c->fd, events);
        }
        if ((events & (EPOLLERR|EPOLLHUP))
             && (events & (EPOLLIN|EPOLLOUT)) == 0) {
            /*
             * if the error events were returned without
             * EPOLLIN or EPOLLOUT, then add these flags
             * to handle the events at least in one active handler
             */
            events |= EPOLLIN|EPOLLOUT;
        }

        if ((events & EPOLLIN) && rev->active) {
            rev->ready = FAST_TRUE;
            if (!rev->handler) {
                fast_log_debug(ep_base->log, FAST_LOG_DEBUG, 0,
                    "epoll_process_events: rev->handler NULL");
                continue;
            }
            if (flags & EVENT_POST_EVENTS) {
                queue = rev->accepted ? &ep_base->posted_accept_events:
                                        &ep_base->posted_events;
                rev->last_instance = instance;
                fast_log_debug(ep_base->log, FAST_LOG_DEBUG, 0,
                    "epoll_process_events: post read event, fd:%d", c->fd);
                queue_insert_tail((queue_t*)queue, &rev->post_queue);
            } else {
                fast_log_debug(ep_base->log, FAST_LOG_DEBUG, 0,
                    "epoll_process_events: read event fd:%d", c->fd);
                rev->handler(rev);
                hd_num++;
            }
        }

        wev = c->write;
        if ((events & EPOLLOUT) && wev->active) {
            wev->ready = FAST_TRUE;
            if (!wev->handler) {
                fast_log_error(ep_base->log, FAST_LOG_WARN, 0,
                    "epoll_process_events: wev->handler NULL");
                continue;
            }
            if (flags & EVENT_POST_EVENTS) {
                fast_log_debug(ep_base->log, FAST_LOG_DEBUG, 0,
                    "epoll_process_events: post write event, fd:%d", c->fd);
                rev->last_instance = instance;
                queue_insert_tail((queue_t*)&ep_base->posted_events, &wev->post_queue);
            } else {
                fast_log_debug(ep_base->log, FAST_LOG_DEBUG, 0,
                    "epoll_process_events: write event fd:%d", c->fd);
                wev->handler(wev);
                hd_num++;
            }
        }
    }

    return FAST_OK;
}
