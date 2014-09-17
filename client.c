#include "protocol.h"

#define INTERVAL (1000*1000*50)

/**
 * struct sockaddr_in serv: 
 *
 * Descriptions
 **/
void
call_tcp(struct sockaddr_in servaddr, bool require_echo, unsigned expected_size,
    unsigned *latency, int *received)
{
    int sockfd;
    char buf[BUF_SIZE];

    struct timespec start;
    struct timespec end;

    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        perror("connect");

    Writen(sockfd, &expected_size, sizeof(unsigned));
    Writen(sockfd, &require_echo, sizeof(bool));
    clock_gettime(CLOCK_MONOTONIC, &start);
    Writen(sockfd, buf, expected_size);
    if (DEBUG) printf("buf sent\n");
    if (require_echo) {
        // measuring latency
        Readn(sockfd, buf, expected_size);
        clock_gettime(CLOCK_MONOTONIC, &end);
        *latency = ((end.tv_sec - start.tv_sec) * 1000000000 +
            end.tv_nsec - start.tv_nsec) / 2;
    } else {
        // measuring througput
        Readn(sockfd, received, sizeof(int));
        clock_gettime(CLOCK_MONOTONIC, &end);
        *latency = ((end.tv_sec - start.tv_sec) * 1000000000 +
            end.tv_nsec - start.tv_nsec);
    }
    close(sockfd);
}


void
call_udp(struct sockaddr_in servaddr, bool require_echo, unsigned expected_size,
    unsigned *latency, int *received)
{
    int sockfd;
    char buf[BUF_SIZE];

    struct timespec start;
    struct timespec end;

    struct SimpleProtocol data;

    data.require_echo = require_echo;
    data.expected_size = expected_size;

    sockfd=socket(AF_INET, SOCK_DGRAM, 0);

    Sendton(sockfd, &data, sizeof(data), 0,
        (struct sockaddr *)&servaddr, sizeof(servaddr));
    clock_gettime(CLOCK_MONOTONIC, &start);
    Sendton(sockfd, buf, expected_size, 0,
        (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (DEBUG) printf("request sent: expected_size=%u, require_echo=%i,  Reading...\n",
        data.expected_size, data.require_echo);
    if (data.require_echo) {
        // measuring latency
        // prevent client from blocking on receiving less data than expected
        setNonblocking(sockfd);
        Readn(sockfd, buf, expected_size);
        clock_gettime(CLOCK_MONOTONIC, &end);
        *latency = ((end.tv_sec - start.tv_sec) * 1000000000 +
            end.tv_nsec - start.tv_nsec) / 2;
    } else {
        // measuring througput
        Readn(sockfd, received, sizeof(int));
        clock_gettime(CLOCK_MONOTONIC, &end);
        *latency = ((end.tv_sec - start.tv_sec) * 1000000000 +
            end.tv_nsec - start.tv_nsec);
        if (DEBUG) printf("SUMMARY: %i of %u sent\n", *received, expected_size);
    }
    close(sockfd);
}


void
call_udp_epoll(struct sockaddr_in servaddr, unsigned expected_size,
    unsigned *latency, int *received, bool is_test_latency)
{
    int sockfd;
    char buf[BUF_SIZE];

    struct timespec start;
    struct timespec end;

    int n;

    struct epoll_event ev, events[MAX_EVENTS];
    int nfds, epollfd;

    struct timespec wait_for_server={.tv_sec=0, .tv_nsec=INTERVAL};

    nanosleep(&wait_for_server, NULL);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (setNonblocking(sockfd) != 0) perror("setNonblocking");

    epollfd = epoll_create(16);
    if (epollfd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    // need not care about blocking
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
        perror("epoll_ctl: sockfd");
        exit(EXIT_FAILURE);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    Sendton(sockfd, buf, expected_size, 0,
        (struct sockaddr *)&servaddr, sizeof(servaddr));

    if (DEBUG) printf("client sent %u\n", expected_size);
    // client needs not to loop
    nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
        perror("epoll_pwait");
        exit(EXIT_FAILURE);
    }

    for (n = 0; n < nfds; ++n) {
        if (events[n].events & EPOLLIN) { // readable
            if (is_test_latency)
                *received = Readn(events[n].data.fd, buf, BUF_SIZE);
            else
                Readn(events[n].data.fd, received, sizeof(int));
            clock_gettime(CLOCK_MONOTONIC, &end);
            if (DEBUG) printf("Sent %u, Received %i\n", expected_size, *received);
        }
    }

    if (is_test_latency)
        *latency = ((end.tv_sec - start.tv_sec) * 1000000000 +
            end.tv_nsec - start.tv_nsec) / 2;
    else
        *latency = (end.tv_sec - start.tv_sec) * 1000000000 +
            end.tv_nsec - start.tv_nsec;
    close(sockfd);
}


#ifdef CLIENT_MAIN
int main(int argc, char**argv)
{
    struct sockaddr_in servaddr;
    unsigned chunk_size[] = {4, 16, 64, 256, 1024, 4*1024, 16*1024, 64*1024,
        256*1024, 512*1024};
    unsigned latency, total_time;
    int received;
    int i;
    int repeat=1;

    if (argc == 4) {
        repeat = atoi(argv[3]);
    } else if (argc != 3) {
        printf("usage: [udp|tcp] <IP address>\n");
        exit(1);
    }

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(argv[2]);
    servaddr.sin_port=htons(PORT);

    while (repeat) {
        if (strcmp(argv[1], "udp") == 0) {
            printf("SIZE\tUDP latency us\tUDP througput MB/s\tLOST\tLOSS rate%%\n");
            for (i=0; i<sizeof(chunk_size) / sizeof(chunk_size[0]); i++) {
                call_udp(servaddr, 1, chunk_size[i], &latency, NULL);
                call_udp(servaddr, 0, chunk_size[i], &total_time, &received);
                printf("%8u\t%8u\t%8.2f\t%8u\t%8.2f\n", chunk_size[i], latency / 1000,
                    chunk_size[i] * 1000000000.0 / total_time / 1024 / 1024, chunk_size[i] - received,
                    (chunk_size[i] - received) * 100.0 / chunk_size[i]);
            }
        } else if (strcmp(argv[1], "udpl") == 0) {
            printf("SIZE\tUDP latency us\tLOST\tLOSS Rate%%\n");
            for (i=0; i<sizeof(chunk_size) / sizeof(chunk_size[0]); i++) {
                call_udp_epoll(servaddr, chunk_size[i], &latency, &received, 1);
                printf("%8u\t%8u\t%8u\t%8.2f\n", chunk_size[i], latency / 1000,
                    chunk_size[i] - received,
                    (chunk_size[i] - received) * 100.0 / chunk_size[i]);
            }
        } else if (strcmp(argv[1], "udpt") == 0) {
            printf("SIZE\tUDP throughput MB/s\tLOST\tLOSS Rate%%\n");
            for (i=0; i<sizeof(chunk_size) / sizeof(chunk_size[0]); i++) {
                call_udp_epoll(servaddr, chunk_size[i], &latency, &received, 0);
                printf("%8u\t%8.2f\t%8u\t%8.2f\n", chunk_size[i],
                    chunk_size[i] * 1000000000.0 / latency / 1024 / 1024,
                    chunk_size[i] - received,
                    (chunk_size[i] - received) * 100.0 / chunk_size[i]);
            }
        } else if (strcmp(argv[1], "tcp") == 0) {
            printf("SIZE\tTCP latency us\tTCP througput MB/s\tLOST\tLOSS rate\n");
            for (i=0; i<sizeof(chunk_size) / sizeof(chunk_size[0]); i++) {
                call_tcp(servaddr, 1, chunk_size[i], &latency, NULL);
                call_tcp(servaddr, 0, chunk_size[i], &total_time, &received);
                printf("%8u\t%8u\t%8.2f\t%8u\t%8.2f\n", chunk_size[i], latency / 1000,
                    chunk_size[i] * 1000000000.0 / total_time / 1024 / 1024, chunk_size[i] - received,
                    (chunk_size[i] - received) * 100.0 / chunk_size[i]);
            }
        }
        repeat--;
    }
}
#endif
