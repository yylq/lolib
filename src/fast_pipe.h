
/*
 * hantianle
 *
 */

#ifndef _FAST_PIPE_H
#define _FAST_PIPE_H

#define PIPE_MAX_SIZE		8192

#include "fast_types.h"

typedef struct pipe_s {
    int        pfd[2];
    size_t     size;
} pipe_t;


int  pipe_open(pipe_t *p);
void pipe_close(pipe_t *p);

#endif

