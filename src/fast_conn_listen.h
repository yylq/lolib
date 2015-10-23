#ifndef _FAST_CONN_LISTEN
#define _FAST_CONN_LISTEN

#include "fast_types.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "fast_string.h"
#include "fast_array.h"
#include "fast_conn.h"
#include "fast_error_log.h"

struct listening_s {
    int                    fd;
    struct sockaddr       *sockaddr;
    socklen_t              socklen;    /* size of sockaddr */
    string_t               addr_text;
    int                    family;
    int                    type;
    int                    backlog;
    int                    rcvbuf;
    int                    sndbuf;
    //handler of accepted connection
    event_handler_pt       handler;
    //array of fast_http_in_port_t, for example
    log_t                 *log;
    //set connection pool size
    size_t                 conn_psize;
    //should be here because of the AcceptEx() preread
    listening_t           *previous;
    //each listening socket has a connection, for read event and others
    conn_t                *connection;
    uint32_t               open:1;
    uint32_t               ignore:1;
    uint32_t               linger:1;
    //inherited from previous process
    uint32_t               inherited:1;
    uint32_t               listen:1;
};

int conn_listening_open(array_t *listening, log_t *log);
listening_t * conn_listening_add(array_t *listening, pool_t *pool, 
    log_t *log, in_addr_t addr, in_port_t port, event_handler_pt handler,
    int rbuff_len, int sbuff_len);
int conn_listening_close(array_t *listening);
int conn_listening_add_event(event_base_t *base, array_t *listening);
int conn_listening_del_event(event_base_t *base, array_t *listening);

#endif
