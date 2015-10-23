#ifndef _FAST_NOTICE_H
#define _FAST_NOTICE_H
#include "fast_types.h"
#include "fast_event.h"
#include "fast_pipe.h"
#include "fast_epoll.h"
typedef struct notice_s notice_t;
typedef int  (*wake_up_ptr)(notice_t *n);
typedef void (*wake_up_hander)(void *data);

struct notice_s {
    pipe_t           channel;
    
    wake_up_ptr      wake_up;
    wake_up_hander   call_back;
    void            *data;
    log_t           *log;
};

int notice_init(event_base_t *base, notice_t *n, wake_up_hander handler, void *data);
int notice_wake_up(notice_t *n);

#endif
