#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>

#include "utils.h"


void err_sys(const char* x)
{
    perror(x);
    exit(1);
}


/* include readn */
ssize_t                     /* Read "n" bytes from a descriptor. */
readn(int fd, void *vptr, size_t n)
{
    size_t  nleft;
    ssize_t nread;
    char    *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;      /* and call read() again */
            else
                return(-1);
        } else if (nread == 0)
            break;              /* EOF */

        nleft -= nread;
        ptr   += nread;
    }
    return (n - nleft);      /* return >= 0 */
}
/* end readn */


ssize_t
Readn(int fd, void *ptr, size_t nbytes)
{
    ssize_t     n;

    if ( (n = readn(fd, ptr, nbytes)) < 0)
        err_sys("readn error");
    return(n);
}


ssize_t
recvfromn(int sockfd, void *buf, size_t len, int flags,
    struct sockaddr *src_addr, socklen_t *addrlen) {

    size_t  nleft;
    ssize_t nread;
    void    *ptr;

    ptr = buf;
    nleft = len;
    while (nleft > 0) {
        if ( (nread = recvfrom(sockfd, ptr, nleft, flags, src_addr, addrlen)) < 0) {
            if (errno == EINTR)
                nread = 0;      /* and call read() again */
            else
                return(-1);
        } else if (nread == 0)
            break;              /* EOF */

        nleft -= nread;
        ptr   += nread;
        if (DEBUG) printf("recvfrom nread: %i, nleft: %u\n", nread, nleft);
    }
    if (DEBUG) printf("recvfromn nleft: %u of %u\n", nleft, len);
    return (len - nleft);      /* return >= 0 */
}


ssize_t
Recvfromn(int sockfd, void *buf, size_t len, int flags,
    struct sockaddr *src_addr, socklen_t *addrlen) {
    ssize_t n;

    if ( (n = recvfromn(sockfd, buf, len, flags, src_addr, addrlen)) < 0)
        err_sys("recvfromn error");
    return(n);
}


ssize_t                         /* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;   /* and call write() again */
            else
                return (-1);    /* error */
         }

         nleft -= nwritten;
         ptr += nwritten;
    }
    return (n);
}


void
Writen(int fd, void *ptr, ssize_t nbytes)
{
    if (writen(fd, ptr, nbytes) != nbytes)
        err_sys("writen error");
}


ssize_t
sendton(int sockfd, const void *buf, size_t len, int flags,
    const struct sockaddr *dest_addr, socklen_t addrlen) {

    size_t nleft;
    ssize_t nwritten;
    const void *ptr;

    ptr = buf;
    nleft = len;
    while (nleft > 0) {
        if ( (nwritten = sendto(sockfd, ptr, MIN(nleft, MAX_UDP_SIZE), flags, dest_addr, addrlen)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;   /* and call write() again */
            else
                return (-1);    /* error */
         }

         nleft -= nwritten;
         ptr += nwritten;
    }
    assert(nleft == 0);
    if (DEBUG) printf("sendto len: %u, nleft: %u\n", len, nleft);
    return (len - nleft);
}


void
Sendton(int sockfd, const void *buf, size_t len, int flags,
    const struct sockaddr *dest_addr, socklen_t addrlen) {
    if (sendton(sockfd, buf, len, flags, dest_addr, addrlen) < (ssize_t)len)
        err_sys("sendton error");
}
