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



#endif
