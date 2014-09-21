#ifndef __UTILS_H
#define __UTILS_H

#include <sys/types.h>
#include <assert.h>

#define MAX_UDP_SIZE 65507
#define MIN(a,b) (((a)<(b))?(a):(b))

#ifndef DEBUG
#define DEBUG 1
#endif

void err_sys(const char* x);
ssize_t readn(int fd, void *vptr, size_t n);
ssize_t Readn(int fd, void *ptr, size_t nbytes);
ssize_t writen(int fd, const void *vptr, size_t n);
void Writen(int fd, void *ptr, ssize_t nbytes);

ssize_t recvfromn(int sockfd, void *buf, size_t len, int flags,
    struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t Recvfromn(int sockfd, void *buf, size_t len, int flags,
    struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t sendton(int sockfd, const void *buf, size_t len, int flags,
    const struct sockaddr *dest_addr, socklen_t addrlen);
void Sendton(int sockfd, const void *buf, size_t len, int flags,
    const struct sockaddr *dest_addr, socklen_t addrlen);

#endif
