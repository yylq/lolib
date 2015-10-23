
/*
 * fast_channel.c
 */
#include "fast_channel.h"
#include "fast_conn.h"
#include "fast_memory.h"


int
channel_write(int socket, channel_t *ch,
    size_t size, log_t *log)
{
    ssize_t             n;
    struct iovec        iov[1];
    struct msghdr       msg;

    union {
        struct cmsghdr  cm;
        char            space[CMSG_SPACE(sizeof(int))];
    } cmsg;

    if (ch->fd == -1) {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;

    } else {
        msg.msg_control = (caddr_t) &cmsg;
        msg.msg_controllen = sizeof(cmsg);

        cmsg.cm.cmsg_len = CMSG_LEN(sizeof(int));
        cmsg.cm.cmsg_level = SOL_SOCKET;
        cmsg.cm.cmsg_type = SCM_RIGHTS;
        /*
        have to use memory_memcpy() instead of simple
        *(int *)CMSG_DATA(&cmsg.cm) = ch->fd;
        because some gcc 4.4 with -O2/3/s optimization issues the warning:
        dereferencing type-punned pointer will break strict-aliasing rules
        */
        //*(int *) CMSG_DATA(&cmsg.cm) = ch->fd;
        memory_memcpy(CMSG_DATA(&cmsg.cm), &ch->fd, sizeof(int));
    }

    msg.msg_flags = 0;


    iov[0].iov_base = (char *) ch;
    iov[0].iov_len = size;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    n = sendmsg(socket, &msg, 0);

    if (n == FAST_ERROR) {
        if (errno == FAST_EAGAIN) {
            return FAST_AGAIN;
        }

        return FAST_ERROR;
    }

    return FAST_OK;
}

int channel_read(int socket, channel_t *ch,
    size_t size, log_t *log)
{
    ssize_t             n;
    struct iovec        iov[1];
    struct msghdr       msg;

    union {
        struct cmsghdr  cm;
        char            space[CMSG_SPACE(sizeof(int))];
    } cmsg;

    iov[0].iov_base = (char *) ch;
    iov[0].iov_len = size;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    msg.msg_control = (caddr_t) &cmsg;
    msg.msg_controllen = sizeof(cmsg);

    n = recvmsg(socket, &msg, 0);

    if (n == FAST_ERROR) {
        if (errno == FAST_EAGAIN) {
            return FAST_AGAIN;
        }

        return FAST_ERROR;
    }

    if (n == 0) {
        return FAST_ERROR;
    }

    if ((size_t) n < sizeof(channel_t)) {
     
        return FAST_ERROR;
    }

    if (ch->command == CHANNEL_CMD_OPEN) {
        if (cmsg.cm.cmsg_len < (socklen_t) CMSG_LEN(sizeof(int))) {
            
            return FAST_ERROR;
        }

        if (cmsg.cm.cmsg_level != SOL_SOCKET || cmsg.cm.cmsg_type != SCM_RIGHTS)
        {
            
            return FAST_ERROR;
        }

        //ch->fd = *(int *) CMSG_DATA(&cmsg.cm);
        memory_memcpy(&ch->fd, CMSG_DATA(&cmsg.cm), sizeof(int));
    }

    return n;
}

void
channel_close(int *fd, log_t *log)
{
    close(fd[0]);
    close(fd[1]);
}
