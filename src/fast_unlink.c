/*
 * hantianle
 */

#include "fast_unlink.h"

int
fast_unlink(uchar_t *path)
{
    return unlink((char *)path);
}
