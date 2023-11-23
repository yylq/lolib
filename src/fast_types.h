#ifndef _FAST_TYPES_H
#define _FAST_TYPES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define FAST_DEBUG               1

/* compile option */
#define _xcdecl
#define _xinline                   inline
#define _xvolatile                 volatile
#define _xlibc_cdecl

#define FAST_MAX_SIZE_T_VALUE    2147483647L

/* sys return value */
#define FAST_OK                  0
#define FAST_ERROR              -1

#define FAST_AGAIN              -2
#define FAST_BUSY               -3
#define FAST_PUSH               -4
#define FAST_DECLINED           -5
#define FAST_ABORT              -6
#define FAST_GO_ON              -7
#define FAST_BADMEM             -8
#define FAST_BUFFER_FULL        -9
#define FAST_CONN_CLOSED        -10
#define FAST_TIME_OUT           -11

#define FAST_TRUE                1
#define FAST_FALSE               0

#define FAST_LINEFEED_SIZE       1
#define FAST_INVALID_FILE       -1
#define FAST_INVALID_PID        -1
#define FAST_FILE_ERROR         -1  


#define FAST_TID_T_FMT           "%ud"    
#define FAST_INT32_LEN           sizeof("-2147483648") - 1          
#define FAST_INT64_LEN           sizeof("-9223372036854775808") - 1 
#define FAST_PTR_SIZE            4

#define MB_SIZE                   1024 * 1024
#define GB_SIZE                   1024 * 1024 * 1024

#if (FAST_PTR_SIZE == 4)
#define FAST_INT_T_LEN FAST_INT32_LEN
#else
#define FAST_INT_T_LEN FAST_INT64_LEN
#endif

#if ((__GNU__ == 2) && (__GNUC_MINOR__ < 8))
#define FAST_MAX_UINT32_VALUE    (uint32_t)0xffffffffLL
#else
#define FAST_MAX_UINT32_VALUE    (uint32_t)0xffffffff
#endif                             /* fast_config.h */

#define LF   (uchar_t) 10
#define CR   (uchar_t) 13 
#define CRLF "\x0d\x0a"            /* fast_core.h */

#include <assert.h>                /* assert() */
#include <sys/types.h>             /* uint32_t, uchar_t, u_short */
#include <sys/time.h>

#if HAVE_INTTYPES_H
#include <inttypes.h>              /* uint32_t uint16_t */
#endif

#include <unistd.h>
#include <stdarg.h>
#include <stdint.h>                /* intptr_t */
#include <stddef.h>                /* offsetof() */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <execinfo.h>

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

#include <glob.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sched.h>
#include <ctype.h>                 /* isspace */
#include <sys/socket.h>

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include <netinet/tcp.h>           /* TCP_NODELAY, TCP_CORK */

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#if HAVE_NETDB_H
#include <netdb.h>
#endif

#include <sys/un.h>

#include <time.h>                  /* tzset() */

#if HAVE_MALLOC
#include <malloc.h>                /* memalign() mallinfo() */
#endif

#if HAVE_LIMITS_H
#include <limits.h>                /* IOV_MAX  CAHR_BIT */
#endif

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <sys/sysctl.h>

#include <crypt.h>
#include <sys/utsname.h>           /* uname() */
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <aio.h>
#include <openssl/md5.h>

#include <pthread.h>               /* thread support */

#ifndef FAST_OFF_T_LEN
#define FAST_OFF_T_LEN          sizeof("-9223372036854775808") - 1
#endif

#define FAST_MD5_INIT            MD5_Init
#define FAST_MD5_UPDATE          MD5_Update
#define FAST_MD5_FINAL           MD5_Final
#define FAST_MD5_KEY_N           16
#define FAST_MD5_KEY_TXT_N       32
#define FAST_MAX_ERROR_STR         2048



/* typedef */
typedef u_char                     uchar_t;
typedef int                        atomic_int_t;
typedef uint32_t                   atomic_uint_t;


typedef int64_t                    rbtree_key;
typedef int64_t                    rbtree_key_int;
typedef int64_t                    rb_msec_t;
typedef int64_t                    rb_msec_int_t;

typedef struct log_s               log_t;
typedef struct pool_s              pool_t;

typedef struct array_s             array_t;
typedef struct string_s            string_t;
typedef struct buffer_s            buffer_t;

typedef struct hashtable_link_s    hashtable_link_t;

typedef struct event_base_s        event_base_t;
typedef struct event_timer_s       event_timer_t;
typedef struct event_s             event_t;
typedef struct listening_s         listening_t;
typedef struct conn_s              conn_t;
typedef struct chain_s             chain_t;


#define MMAP_PROT          PROT_READ | PROT_WRITE
#define MMAP_FLAG          MAP_PRIVATE | MAP_ANONYMOUS

#define ALIGN_POINTER       sizeof(void *)

#define ALIGNMENT(x, align) \
    (((x) % (align)) ? ((x) + ((align) - (x) % (align))) : (x))

#define fast_sys_open(a,b,c)     open((char *)a, b, c)
#define fast_open_fd(a,b)        open((char *)a, b)
#define fast_write_fd            write
#define fast_read_fd             read
#define fast_delete_file(s)      unlink((char *)s)
#define fast_lseek_file		     lseek

typedef ssize_t  (*sysio_recv_pt)(conn_t *c, uchar_t *buf, size_t size);
typedef ssize_t  (*sysio_recv_chain_pt)(conn_t *c, chain_t *in);
typedef ssize_t  (*sysio_send_pt)(conn_t *c, uchar_t *buf, size_t size);
typedef chain_t *(*sysio_send_chain_pt)(conn_t *c, chain_t *in, size_t limit);
typedef chain_t *(*sysio_sendfile_pt)(conn_t *c, chain_t *in, int fd, size_t limit);


#ifndef FAST_PAGE_SIZE
    #if defined(_SC_PAGESIZE)
        #define FAST_PAGE_SIZE sysconf(_SC_PAGESIZE)
    #elif defined(_SC_PAGE_SIZE)
        #define FAST_PAGE_SIZE sysconf(_SC_PAGE_SIZE)
    #else
        #define FAST_PAGE_SIZE (4096)
    #endif
#endif

#ifndef FAST_DEBUG
#define FAST_DEBUG 1
#endif

#define fast_log_error(log, level, args...) \
    error_log_core(log, level, __FILE__, __LINE__, args)
    
#if FAST_DEBUG
#define fast_log_debug(log, level, args...) \
    error_log_debug_core(log, level,  __FILE__, __LINE__, args)
#else
#define fcache_log_debug(log, level, args...) {}
#endif


#endif

