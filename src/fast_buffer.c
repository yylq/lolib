#include "fast_buffer.h"
#include "fast_memory.h"
#include "fast_memory_pool.h"


buffer_t *
buffer_alloc(pool_t *pool)
{
    return pool_alloc(pool, sizeof(buffer_t));
}

buffer_t *
buffer_create(pool_t *pool, size_t size)
{
    buffer_t *b = NULL;

    if (size == 0) {
        return NULL;
    }
    if (!pool) {
        b = (buffer_t *)memory_alloc(sizeof(buffer_t));
        if (!b) {
            return NULL;
        }
        b->start = (uchar_t *)memory_alloc(size);
        if (!b->start) {
            memory_free(b, sizeof(buffer_t));
            return NULL;
        }
		b->temporary = FAST_FALSE;
    } else {
        b = (buffer_t *)buffer_alloc(pool);
        if (!b) {
            return NULL;
        }
        b->start = (uchar_t *)pool_alloc(pool, size);
        if (!b->start) {
            return NULL;
        }
       b->temporary = FAST_TRUE;
    }
    
    b->pos = b->start;
    b->last = b->start;
    b->end = b->last + size;
    b->memory = FAST_TRUE;
    b->in_file = FAST_FALSE;
	
    return b;
}
void      buffer_shrink(buffer_t* buf)
{
	int blen;
	if (!buf) {
        return;
    }
	if (buf->start == buf->pos) {
		return ;
	}
	blen = buffer_size(buf);
	if (!blen) {
		buffer_reset(buf);
		return ;
	}
	memmove(buf->start,buf->pos,blen);
	buf->pos = buf->start;
	buf->last = buf->pos+blen;
	return ;	  
}
void
buffer_free(buffer_t *buf)
{
    if (!buf || buf->temporary) {
        return;
    }
    memory_free(buf->start, buf->end - buf->start);
    memory_free(buf, sizeof(buffer_t));
}

