#include "fast_sysio.h"
#include "fast_chain.h"
#include "fast_error_log.h"
static ssize_t sysio_writev_iovs(conn_t *c,
    sysio_vec *iovs, int count);
static int sysio_pack_chain_to_iovs(sysio_vec *iovs,
    int iovs_count, chain_t *in, size_t *last_size, size_t limit);

sysio_t linux_io = {
    //read
    sysio_unix_recv,
    sysio_readv_chain,
    sysio_udp_unix_recv,
    //write
    sysio_unix_send,
    sysio_writev_chain,
    sysio_sendfile_chain,
    0
};

ssize_t
sysio_unix_recv(conn_t *c, uchar_t *buf, size_t size)
{
    ssize_t  n = 0;

    if (!c) {
        fast_log_error(c->log, FAST_LOG_WARN, 0,
            "sysio_unix_recv: c is NULL");
        return FAST_ERROR;
    }
    if (!buf) {
        fast_log_error(c->log, FAST_LOG_WARN, 0,
            "sysio_unix_recv: buf is NULL");
        return FAST_ERROR;
    }
    for (;;) {
        errno = 0;
        n = recv(c->fd, buf, size, 0);
        fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
            "sysio_unix_recv: recv:%d, buf size:%d, fd:%d",
            n, size, c->fd);
        //n > 0
        if (n > 0) {
            return n;
        }
        //n == 0
        if (n == 0) {
            return n;
        }
        //n < 0
        if (errno == FAST_EINTR) {
            continue;
        }
        if (errno == FAST_EAGAIN) {
            fast_log_debug(c->log,FAST_LOG_DEBUG, 0,
                "sysio_unix_recv: not ready");
            return FAST_AGAIN;
        }
        //recv error
        fast_log_error(c->log, FAST_LOG_ERROR, errno,
            "sysio_unix_recv: error, fd:%d", c->fd);
        return FAST_ERROR;
    }

    //can't be here
    return FAST_OK;
}

ssize_t
sysio_readv_chain(conn_t *c, chain_t *chain)
{
    int           i = 0;
    uchar_t       *prev = NULL;
    ssize_t       n = 0;
    ssize_t       size = 0;
    struct iovec  iovs[FAST_IOVS_REV];

    //coalesce the neighbouring bufs
    while (chain && i < FAST_IOVS_REV) {
        if (prev == chain->buf->last) {
            iovs[i - 1].iov_len += chain->buf->end - chain->buf->last;
            fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
                "sysio_readv_chain: readv prev == chain->buf->last");
        } else {
            iovs[i].iov_base = (void *) chain->buf->last;
            iovs[i].iov_len = chain->buf->end - chain->buf->last;
            fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
                "sysio_readv_chain: readv iov len %d", iovs[i].iov_len);
            i++;
        }
        size += chain->buf->end - chain->buf->last;
        prev = chain->buf->end;
        chain = chain->next;
    }

    for (;;) {
        errno = 0;
        n = readv(c->fd, iovs, i);
        fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
            "sysio_readv_chain: read:%d buf size:%d", n, size);
        //n > 0
        if (n > 0) {
            return n;
        }
        //n == 0;
        if (n == 0) {
            return n;
        }
        //n < 0
        if (errno == FAST_EAGAIN) {
            fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
                "sysio_readv_chain: not ready");
            return FAST_AGAIN;
        }
        if (errno == FAST_EINTR) {
            continue;
        }
        fast_log_error(c->log, FAST_LOG_WARN, errno,
            "sysio_readv_chain: read error");
        return FAST_ERROR;
    } 
    //can't be here
    return FAST_OK;
}

ssize_t
sysio_udp_unix_recv(conn_t *c, uchar_t *buf, size_t size)
{
    ssize_t n = 0;

    for (;;) {
        errno = 0;
        n = recv(c->fd, buf, size, 0);
        fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
            "sysio_udp_unix_recv: recv:%d buf_size:%d fd:%d", n, size, c->fd);
        //n >= 0
        if (n >= 0) {
            return n;
        }
        //n < 0
        if (errno == FAST_EINTR) {
            continue;
        }
        if (errno == FAST_EAGAIN) {
            fast_log_debug(c->log, FAST_LOG_DEBUG, errno,
                "sysio_udp_unix_recv: not ready");
            return FAST_AGAIN;
        }
        //error
        fast_log_error(c->log, FAST_LOG_WARN, errno,
            "sysio_udp_unix_recv: recv error");
        return FAST_ERROR;
    }
    
    return FAST_OK;
}

ssize_t
sysio_unix_send(conn_t *c, uchar_t *buf, size_t size)
{
    ssize_t  n;
    event_t *wev;

    wev = c->write;

    for ( ;; ) {
        errno = 0;
        n = send(c->fd, buf, size, 0);
        fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
            "sysio_unix_send: send:%d buf_size:%d fd:%d",n, size, c->fd);
        //n > 0
        if (n > 0) {
            if (n < (ssize_t) size) {
                wev->ready = 0;
            }
            return n;
        }
        //n == 0
        if (n == 0) {
            fast_log_error(c->log, FAST_LOG_ALERT, errno,
                "sysio_unix_send: send zero, not ready");
            wev->ready = 0;
            return n;
        }
        //n < 0
        if (errno == FAST_EINTR) {
            continue;
        }
        if (errno == FAST_EAGAIN) {
            wev->ready = 0;
            fast_log_debug(c->log, FAST_LOG_DEBUG, errno,
                "sysio_unix_send: not ready");
            return FAST_AGAIN;
        }
        //error
        fast_log_error(c->log, FAST_LOG_WARN, errno,
            "sysio_unix_send: send error");
        return FAST_ERROR;
    }
    //can't be here
    return FAST_OK;
}

//TODO: if limit > sendbuf, please set it to sendbuf
chain_t *
sysio_writev_chain(conn_t *c, chain_t *in, size_t limit)
{
    int           pack_count = 0;
    size_t        packall_size = 0;
    size_t        last_size = 0;
    ssize_t       sent_size = 0;
    chain_t      *cl = NULL;
    event_t      *wev = NULL;
    sysio_vec     iovs[FAST_IOVS_MAX];

    if (!in) {
        return NULL;
    }
    if (!c) {
        return FAST_CHAIN_ERROR;
    }
    wev = c->write;
    if (!wev) {
        return FAST_CHAIN_ERROR;
    }
    if (!wev->ready) {
        return in;
    }
    //the maximum limit size is the 2G - page size
    if (limit == 0 || limit > FAST_MAX_LIMIT) {
        limit = FAST_MAX_LIMIT;
    }
    
    while (in && packall_size < limit) {
        last_size = packall_size;
		if (in->buf->memory == FAST_FALSE) {
            fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
                "%s, file data break", __func__); 
			break;
		}
        pack_count = sysio_pack_chain_to_iovs(iovs,
            FAST_IOVS_MAX, in, &packall_size, limit);
        if (pack_count == 0) {
            fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
                "%s, pack_count zero", __func__);
            return NULL;
        }
        fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
            "sysio_writev_chain: pack_count:%d, packall_size:%ul",
            pack_count, packall_size);
        sent_size = sysio_writev_iovs(c, iovs, pack_count);
        fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
            "sysio_writev_chain: write:%d, iovs_size:%ul, sent:%d",
            sent_size, packall_size - last_size, c->sent);
		
        if (sent_size > 0) {
            c->sent += sent_size;
            cl = chain_write_update(in, sent_size);
            if (packall_size - last_size > (size_t)sent_size) {
                //wev->ready = 0;
                //return cl;
                fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
                    "sysio_writev_chain: write size < iovs_size");
            }
            if (packall_size >= limit) {
                fast_log_debug(c->log, FAST_LOG_DEBUG, errno,
                    "sysio_writev_chain: writev to limit");
                return cl;
            }
            in = cl;
            continue;
        }
        if (sent_size == FAST_AGAIN) {
            fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
                "%s, writev_chain again", __func__); 
            wev->ready = 0;
            return in;
        }
        if (sent_size == FAST_ERROR) {
            fast_log_debug(c->log, FAST_LOG_DEBUG, errno,
                "sysio_writev_chain: writev error, fd:%d", c->fd);
            return FAST_CHAIN_ERROR;
        }
    }
    
    return in;
}

/*
 * On Linux up to 2.4.21 sendfile() (syscall #187) works with 32-bit
 * offsets only, and the including <sys/sendfile.h> breaks the compiling,
 * if off_t is 64 bit wide.  So we use own sendfile() definition, where offset
 * parameter is int32_t, and use sendfile() for the file parts below 2G only,
 * see src/os/unix/fast_linux_config.h
 *
 * Linux 2.4.21 has the new sendfile64() syscall #239.
 *
 * On Linux up to 2.6.16 sendfile() does not allow to pass the count parameter
 * more than 2G-1 bytes even on 64-bit platforms: it returns EINVAL,
 * so we limit it to 2G-1 bytes.
 */

chain_t *
sysio_sendfile_chain(conn_t *c, chain_t *in, int fd, size_t limit)
{
	int           rc = 0;
    size_t        pack_size = 0;
    event_t      *wev = NULL;	
    size_t        sent = 0;

	if (!in) {
        return NULL;
    }
    if (!c) {
        return FAST_CHAIN_ERROR;
    }
    wev = c->write;
    if (!wev) {
        return FAST_CHAIN_ERROR;
    }
	if (!wev->ready) {
        return in;
    }
    //the maximum limit size is the 2G - page size
    if (limit == 0 || limit > FAST_MAX_LIMIT) {
        limit = FAST_MAX_LIMIT;
    }
    fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
        "%s, limit:%d, fd:%d", __func__, limit, fd);
    while (in && sent < limit) {
		if (in->buf->memory == FAST_TRUE) {
            fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
                "%s, memory data break", __func__); 
			break;
		}
		pack_size = buffer_size(in->buf);
		if (pack_size == 0) {
            fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
                "%s, pack size zero", __func__); 
			in = in->next;
			continue;
		}
        if(sent + pack_size > limit) {
           pack_size = limit - sent;
        }
        fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
            "%s, limit:%d already sent:%d pack_size:%d, file_pos:%l",
            __func__, limit, sent, pack_size, in->buf->file_pos);
		rc = sendfile(c->fd, fd, &in->buf->file_pos, pack_size);
		if (rc == FAST_ERROR) {
			if (errno == FAST_EAGAIN) {
                fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
                    "%s, sendfile again",__func__); 
				wev->ready = 0;
				return in;
		    } else if (errno == FAST_EINTR) {
                fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
                    "%s, sendfile interupt, continue",__func__);
	            continue;
	        } 
	    	fast_log_error(c->log, FAST_LOG_ERROR, errno,
	        	"%s: sendfile error, fd:%d, fd:%d", __func__, c->fd, fd);
			return FAST_CHAIN_ERROR;
		}

        if (!rc) {
            fast_log_error(c->log, FAST_LOG_ALERT, 0,
                "%s: sendfile reture 0, file pos might have error", __func__);
            return FAST_CHAIN_ERROR;
        }
        
        fast_log_debug(c->log, FAST_LOG_DEBUG, 0,
            "%s: fd:%d, sendfile:%d, pack_size:%d, c->sent:%d, file_pos:%l "
            "remain buf size:%l", __func__, c->fd, rc, pack_size, c->sent,
            in->buf->file_pos, buffer_size(in->buf));
        if (rc > 0) {
            c->sent += rc;
            sent += rc;
			if (buffer_size(in->buf) == 0) {
				in = in->next;
			}
        }
    }
 
    return in;
}

static int
sysio_pack_chain_to_iovs(sysio_vec *iovs, int iovs_count,
    chain_t *in, size_t *last_size, size_t limit)
{
    int      i = 0;
    ssize_t  bsize = 0;
    uchar_t  *last_pos = NULL;

    if (!iovs || !in || !last_size) {
        return i;
    }
    while (in && i < iovs_count && *last_size < limit) {
		if (in->buf->memory == FAST_FALSE) {
			break;
		}
        bsize = buffer_size(in->buf);
        if (bsize <= 0) {
            in = in->next;
            continue;
        }
        if (*last_size + bsize > limit) {
            bsize = limit - *last_size;
        }
        if (last_pos != in->buf->pos) {
            iovs[i].iov_base = in->buf->pos;
            iovs[i].iov_len = bsize;
            i++;
        } else {
            iovs[i - 1].iov_len += bsize;
        }
        *last_size += bsize;
        last_pos = in->buf->pos;
        in = in->next;
    }

    return i;
}

static ssize_t
sysio_writev_iovs(conn_t *c, sysio_vec *iovs, int count)
{
    ssize_t rc = FAST_ERROR;
    
    if (!c || !iovs || count <= 0) {
        return rc;
    }
    
    for (;;) {
        errno = 0;
        rc = writev(c->fd, iovs, count);
        if (rc > 0) {
            return rc;
        }
        if (rc == FAST_ERROR) {
            if (errno == FAST_EINTR) {
                continue;
            }
            if (errno == FAST_EAGAIN) {
                return FAST_AGAIN;
            }
            return FAST_ERROR;
        }
        if (rc == 0) {
            return FAST_ERROR;
        }
    }

    return FAST_ERROR;
}

