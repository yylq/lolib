#ifndef _FAST_CONNECTION_H
#define _FAST_CONNECTION_H

#include "fast_types.h"
#include "fast_sysio.h"
#include "fast_string.h"
#include "fast_event.h"
#include "fast_error_log.h"

//#define CONN_DEFAULT_RCVBUF    -1
//#define CONN_DEFAULT_SNDBUF    -1
#define CONN_DEFAULT_RCVBUF    (64<<10)
#define CONN_DEFAULT_SNDBUF    (64<<10)
#define CONN_DEFAULT_POOL_SIZE 2048
#define CONN_DEFAULT_BACKLOG   2048

typedef struct conn_peer_s conn_peer_t;

enum {
    CONN_TCP_NODELAY_UNSET = 0,
    CONN_TCP_NODELAY_SET,
    CONN_TCP_NODELAY_DISABLED
};

enum {
    CONN_TCP_NOPUSH_UNSET = 0,
    CONN_TCP_NOPUSH_SET,
    CONN_TCP_NOPUSH_DISABLED
};

enum {
    CONN_ERROR_NONE = 0,
    CONN_ERROR_REQUEST,
    CONN_ERROR_EOF
};

struct conn_s {
    int                    fd;
    void                  *next;
    void                  *conn_data;
    event_t               *read;
    event_t               *write;
    sysio_recv_pt          recv;
    sysio_send_pt          send;
    sysio_recv_chain_pt    recv_chain;
    sysio_send_chain_pt    send_chain;
    sysio_sendfile_pt      sendfile_chain;
    listening_t           *listening;
    size_t                 sent;
    pool_t                *pool;
    struct sockaddr       *sockaddr;
    socklen_t              socklen;
    string_t               addr_text;
    struct timeval         accept_time;
    uint32_t               error:1;
    uint32_t               sendfile:1;
    uint32_t               sndlowat:1;
    uint32_t               tcp_nodelay:2;
    uint32_t               tcp_nopush:2;
    event_timer_t         *ev_timer;
    event_base_t          *ev_base;  
    log_t                 *log;
};

struct conn_peer_s {
    conn_t                *connection;
    struct sockaddr       *sockaddr;
    socklen_t              socklen;
    string_t              *name;
    int                    rcvbuf;
};

#define FAST_EPERM         EPERM
#define FAST_ENOENT        ENOENT
#define FAST_ESRCH         ESRCH
#define FAST_EINTR         EINTR
#define FAST_ECHILD        ECHILD
#define FAST_ENOMEM        ENOMEM
#define FAST_EACCES        EACCES
#define FAST_EBUSY         EBUSY
#define FAST_EEXIST        EEXIST
#define FAST_ENOTDIR       ENOTDIR
#define FAST_EISDIR        EISDIR
#define FAST_EINVAL        EINVAL
#define FAST_ENOSPC        ENOSPC
#define FAST_EPIPE         EPIPE
#define FAST_EAGAIN        EAGAIN
#define FAST_EINPROGRESS   EINPROGRESS
#define FAST_EADDRINUSE    EADDRINUSE
#define FAST_ECONNABORTED  ECONNABORTED
#define FAST_ECONNRESET    ECONNRESET
#define FAST_ENOTCONN      ENOTCONN
#define FAST_ETIMEDOUT     ETIMEDOUT
#define FAST_ECONNREFUSED  ECONNREFUSED
#define FAST_ENAMETOOLONG  ENAMETOOLONG
#define FAST_ENETDOWN      ENETDOWN
#define FAST_ENETUNREACH   ENETUNREACH
#define FAST_EHOSTDOWN     EHOSTDOWN
#define FAST_EHOSTUNREACH  EHOSTUNREACH
#define FAST_ENOSYS        ENOSYS
#define FAST_ECANCELED     ECANCELED
#define FAST_ENOMOREFILES  0

#define FAST_SOCKLEN       512

#define conn_nonblocking(s)  fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK)
#define conn_blocking(s)     fcntl(s, F_SETFL, fcntl(s, F_GETFL) & ~O_NONBLOCK)

//void conn_accept(event_t *ev);
int  conn_connect_peer(conn_peer_t *pc, event_base_t *ep_base);
conn_t *conn_get_from_mem(int s);
void conn_free_mem(conn_t *c);

void conn_set_default(conn_t *c, int s);
void conn_close(conn_t *c);
void conn_release(conn_t *c);

//socket
int  conn_tcp_nopush(int s);
int  conn_tcp_push(int s);
int  conn_tcp_nodelay(int s);
int  conn_tcp_delay(int s);

#endif

