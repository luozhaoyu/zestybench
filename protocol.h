#ifndef __PROTOCOL_H
#define __PROTOCOL_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>

#include "utils.h"

#define PORT 32100
#define BUF_SIZE (512*1024 + 1)
#define MAX_EVENTS 32


struct SimpleProtocol {
    uint32_t expected_size;
    int8_t require_echo;
};


struct SimpleState {
    struct SimpleProtocol protocol;
    int fd;
};


int setNonblocking(int fd)
{
    int flags;

    /* If they have O_NONBLOCK, use the Posix way to do it */
#if defined(O_NONBLOCK)
    /* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    /* Otherwise, use the old way of doing it */
    flags = 1;
    return ioctl(fd, FIONBIO, &flags);
#endif
};

#endif
