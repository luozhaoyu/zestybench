#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

#include <sys/wait.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/un.h>

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


/**
 * void: 
 *
 * Descriptions
 **/
void
get_resolution_of_clock_gettime(void)
{
    struct timespec res;
    struct timespec start;
    struct timespec end;
    //register int i=0;

    if (clock_getres(CLOCK_MONOTONIC, &res) == 0) {
        printf("clock_getres: tv_sec=%ld, tv_nsec=%ld\n", res.tv_sec, res.tv_nsec);
    } else {
        perror("clock_getres != 0");
    }

    clock_gettime(CLOCK_REALTIME, &start);
    clock_gettime(CLOCK_REALTIME, &end);
    printf("CLOCK_REALTIME resolution: tv_sec=%ld, tv_nsec=%ld\n",
        end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);

    clock_gettime(CLOCK_MONOTONIC, &start);
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("CLOCK_MONOTONIC resolution: tv_sec=%ld, tv_nsec=%ld\n",
        end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    printf("CLOCK_MONOTONIC_RAW resolution: tv_sec=%ld, tv_nsec=%ld\n",
        end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
    printf("CLOCK_PROCESS_CPUTIME_ID resolution: tv_sec=%ld, tv_nsec=%ld\n",
        end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);

    clock_gettime(CLOCK_MONOTONIC, &start);
    getpid();
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("CLOCK_MONOTONIC getpid(): tv_sec=%ld, tv_nsec=%ld\n",
        end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);

    clock_gettime(CLOCK_MONOTONIC, &start);
    getuid();
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("CLOCK_MONOTONIC getuid(): tv_sec=%ld, tv_nsec=%ld\n",
        end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);
}

/**
 * unsigned: expected_size
 *
 * Descriptions
 **/
long unsigned
pipe_latency(unsigned expected_size, bool debug)
{
    int pipe_go[2], pipe_come[2];
    pid_t child_pid;
    char buf[512*1024 + 1];
    unsigned sent=0, recieved=0, transferred;

    struct timespec start;
    struct timespec end;

    memset(buf, 0, 512*1024 + 1);
    pipe(pipe_go);
    pipe(pipe_come);
    child_pid = fork();

    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (child_pid == 0) { // this is child
        close(pipe_go[1]);
        close(pipe_come[0]);

        if (debug) printf("child is reading %u...\n", expected_size);
        // read first
        while (recieved < expected_size) {
            transferred = read(pipe_go[0], buf + transferred, expected_size - recieved);
            recieved += transferred;
        }
        if (debug) printf("child is sending %u...\n", expected_size);
        // then send back
        while (sent < expected_size) {
            transferred = write(pipe_come[1], buf + transferred, expected_size - sent);
            sent += transferred;
        }

        close(pipe_go[0]);
        close(pipe_come[1]);
        if (debug) printf("child is exiting %u...\n", expected_size);
        _exit(EXIT_SUCCESS);
    } else { // this is parent
        close(pipe_go[0]);
        close(pipe_come[1]);

        clock_gettime(CLOCK_MONOTONIC, &start);
        if (debug) printf("parent is writing %u...\n", expected_size);
        // write first
        while (sent < expected_size) {
            transferred = write(pipe_go[1], buf + transferred, expected_size - sent);
            sent += transferred;
        }

        if (debug) printf("parent is reading %u...\n", expected_size);
        // then read
        while (recieved < expected_size) {
            transferred = read(pipe_come[0], buf + transferred, expected_size - recieved);
            recieved += transferred;
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        close(pipe_go[1]);
        close(pipe_come[0]);
        if (debug) printf("parent is waiting after %u...\n", expected_size);
        wait(NULL);
        if (debug) printf("parent is returning after %u...\n", expected_size);
        return ((end.tv_sec - start.tv_sec) * 1000000000 + end.tv_nsec - start.tv_nsec) / 2;
    }
}


long unsigned
pipe_throughput(unsigned expected_size, bool debug)
{
    int pipe_go[2], pipe_come[2];
    pid_t child_pid;
    char buf[512*1024 + 1];
    unsigned sent=0, recieved=0, transferred;

    struct timespec start;
    struct timespec end;

    memset(buf, 0, 512*1024 + 1);
    pipe(pipe_go);
    pipe(pipe_come);
    child_pid = fork();

    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (child_pid == 0) { // this is child
        close(pipe_go[1]);
        close(pipe_come[0]);

        if (debug) printf("child is reading %u...\n", expected_size);
        // read first
        while (recieved < expected_size) {
            transferred = read(pipe_go[0], buf + transferred, expected_size - recieved);
            recieved += transferred;
        }
        if (debug) printf("child is sending %u...\n", expected_size);
        // then send back
        transferred = write(pipe_come[1], "F", 1);

        close(pipe_go[0]);
        close(pipe_come[1]);
        if (debug) printf("child is exiting %u...\n", expected_size);
        _exit(EXIT_SUCCESS);
    } else { // this is parent
        close(pipe_go[0]);
        close(pipe_come[1]);

        clock_gettime(CLOCK_MONOTONIC, &start);
        if (debug) printf("parent is writing %u...\n", expected_size);
        // write first
        while (sent < expected_size) {
            transferred = write(pipe_go[1], buf + transferred, expected_size - sent);
            sent += transferred;
        }

        if (debug) printf("parent is reading %u...\n", expected_size);
        // then read
        transferred = read(pipe_come[0], buf, 1);
        if (buf[0] == 'F') {
            clock_gettime(CLOCK_MONOTONIC, &end);
        } else {
            printf("ERROR: recieved char is %s!\n", buf);
        }

        close(pipe_go[1]);
        close(pipe_come[0]);
        if (debug) printf("parent is waiting after %u...\n", expected_size);
        wait(NULL);
        if (debug) printf("parent is returning after %u...\n", expected_size);
        return (end.tv_sec - start.tv_sec) * 1000000000 + end.tv_nsec - start.tv_nsec;
    }
}


/**
 * unsigned expected_size, bool debug: 
 *
 * Descriptions
 **/
long unsigned
tcp_latency(unsigned expected_size, bool debug)
{
    int listenfd, connfd;
    struct sockaddr_un servaddr, cliaddr;
    socklen_t cliaddrlen;
    pid_t child_pid;
    unsigned transferred=0;
    char buf[512*1024 + 1];

    struct timespec start;
    struct timespec end;
    struct timespec wait_for_server={.tv_sec=0, .tv_nsec=1000*1000};

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strcpy(servaddr.sun_path, "zestybench.so");

    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
    } else if (child_pid == 0) {
        // this is child, as server
        listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
        //if (setsockopt(listenfd, SOL_TCP, TCP_NODELAY, &optval, sizeof(optval)) < 0)
        //if (setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) < 0) {
        //    perror("setsockopt");
        //}

        // remove existed socket
        unlink(servaddr.sun_path);

        if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
            perror("bind");

        listen(listenfd, 128);
        cliaddrlen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddrlen);
        if (connfd >= 0) {
            // first recieve
            if ((transferred=Readn(connfd, buf, expected_size)) < expected_size)
                printf("only read %u, %u in total", transferred, expected_size);
            // then send
            Writen(connfd, buf, expected_size);
        } else {
            perror("accept");
        }
        close(listenfd);
        close(connfd);
        _exit(EXIT_SUCCESS);
    } else {
        // this is parent
        connfd = socket(AF_UNIX, SOCK_STREAM, 0);

        while (connect(connfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
            nanosleep(&wait_for_server, NULL);
        }
        // first send
        clock_gettime(CLOCK_MONOTONIC, &start);
        Writen(connfd, buf, expected_size);
        // then recieve
        if ((transferred=Readn(connfd, buf, expected_size)) < expected_size)
            printf("only read %u, %u in total", transferred, expected_size);
        clock_gettime(CLOCK_MONOTONIC, &end);
        if (debug) printf("child is sending %u...\n", expected_size);
        close(connfd);
        wait(NULL);
        return ((end.tv_sec - start.tv_sec) * 1000000000 + end.tv_nsec - start.tv_nsec) / 2;
    }
    return 0;
}


long unsigned
tcp_throughput(unsigned expected_size, bool debug)
{
    int listenfd, connfd;
    struct sockaddr_un servaddr, cliaddr;
    socklen_t cliaddrlen;
    pid_t child_pid;
    unsigned transferred=0;
    char buf[512*1024 + 1];

    struct timespec start;
    struct timespec end;
    struct timespec wait_for_server={.tv_sec=0, .tv_nsec=1000*1000};

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strcpy(servaddr.sun_path, "zestybench.so");

    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
    } else if (child_pid == 0) {
        // this is child, as server
        listenfd = socket(AF_UNIX, SOCK_STREAM, 0);

        // remove existed socket
        unlink(servaddr.sun_path);

        if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
            perror("bind");

        listen(listenfd, 128);
        cliaddrlen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddrlen);
        if (connfd >= 0) {
            // first recieve
            if ((transferred=Readn(connfd, buf, expected_size)) < expected_size)
                printf("only read %u, %u in total", transferred, expected_size);
            // then send
            buf[0] = 'F';
            Writen(connfd, buf, 1);
        } else {
            perror("accept");
        }
        close(listenfd);
        close(connfd);
        _exit(EXIT_SUCCESS);
    } else {
        // this is parent
        connfd = socket(AF_UNIX, SOCK_STREAM, 0);

        while (connect(connfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
            nanosleep(&wait_for_server, NULL);
        }
        // first send
        clock_gettime(CLOCK_MONOTONIC, &start);
        Writen(connfd, buf, expected_size);
        // then recieve
        if (!(Readn(connfd, buf, 1) == 1 && buf[0] == 'F')) {
            perror("Readn ack");
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        if (debug) printf("child is sending %u...\n", expected_size);
        close(connfd);
        wait(NULL);
        return (end.tv_sec - start.tv_sec) * 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    return 0;
}


int main()
{
    //get_resolution_of_clock_gettime();
    unsigned chunk_size[10] = {4, 16, 64, 256, 1024, 4*1024, 16*1024, 64*1024,
        256*1024, 512*1024};
    long unsigned t;
    int i;

    printf("SIZE\tPIPE latency\tPIPE throughput\tTCP latency\tTCP throughput\n");
    for (i=0; i<10; i++) {
        printf("%8u\t", chunk_size[i]);
        t = pipe_latency(chunk_size[i], 0);
        printf("%8.2fus\t", t / 1000.0);
        t = pipe_throughput(chunk_size[i], 0);
        printf("%8.2fMB/s\t", chunk_size[i] * 1000000000.0 / t / 1024 / 1024);
        t = tcp_latency(chunk_size[i], 0);
        printf("%8.2fus\t", t / 1000.0);
        t = tcp_throughput(chunk_size[i], 0);
        printf("%8.2fMB/s\t\n", chunk_size[i] * 1000000000.0 / t / 1024 / 1024);
    }
    return 0;
}
