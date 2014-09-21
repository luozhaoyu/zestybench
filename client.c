#include "protocol.h"

#define INTERVAL (1000*1000*50)
#define EPOLL_MILLI_TIMEOUT 10

/**
 * struct sockaddr_in serv: 
 *
 * Descriptions
 **/
void
call_tcp(struct sockaddr_in servaddr, bool require_echo, unsigned expected_size,
    unsigned *latency, size_t *received)
{
    int sockfd;
    char buf[BUF_SIZE];

    struct timespec start;
    struct timespec end;
    int optval=1;

    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if (setsockopt(sockfd, IPPROTO_IP, TCP_NODELAY, &optval, sizeof(optval)) != 0)
        perror("setsockopt TCP_NODELAY");
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        perror("connect");

    struct timespec wait_for_server={.tv_sec=0, .tv_nsec=INTERVAL};
    nanosleep(&wait_for_server, NULL);

    Writen(sockfd, &expected_size, sizeof(unsigned));
    Writen(sockfd, &require_echo, sizeof(bool));
    // could ignore the header time
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
    unsigned *latency, size_t *received)
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
call_udpl_epoll(struct sockaddr_in servaddr, unsigned expected_size,
    unsigned *latency, size_t sender_buf_size)
{
    int sockfd;
    char buf[BUF_SIZE];

    struct timespec start;
    struct timespec end;

    int n, i;
    size_t sent_size=0;
    size_t max_send_size=0;

    struct epoll_event ev, events[MAX_EVENTS];
    int nfds, epollfd;

    struct timespec wait_for_server={.tv_sec=0, .tv_nsec=INTERVAL};

    assert(sender_buf_size <= BUF_SIZE);

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

    max_send_size = MIN(MAX_UDP_SIZE, MIN(expected_size, sender_buf_size));
    clock_gettime(CLOCK_MONOTONIC, &start);
    // send something first
    n = sendton(sockfd, buf, max_send_size, 0,
        (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (n < 0)
        perror("client sendto");

    while (sent_size < expected_size) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_pwait");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < nfds; ++i) {
            if (events[i].events & EPOLLIN) { // readable
                n = Readn(events[i].data.fd, buf, BUF_SIZE);
                sent_size += n; // only the echoed size is effective
                if (DEBUG) printf("%u / %u<<<<%u / %u echo rate\n", n, MIN(max_send_size, expected_size - sent_size), sent_size, expected_size);
                if (sendton(sockfd, buf, MIN(max_send_size, expected_size - sent_size), 0,
                    (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
                    perror("client sendto");
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    *latency = ((end.tv_sec - start.tv_sec) * 1000000000 +
        end.tv_nsec - start.tv_nsec) / 2;
    close(sockfd);
}


void
call_udpt_epoll(struct sockaddr_in servaddr, unsigned expected_size,
    unsigned *latency, size_t sender_buf_size, size_t *received)
{
    int sockfd;
    char buf[BUF_SIZE];

    struct timespec start;
    struct timespec end;

    int n, i, ret;
    size_t sent_size=0;
    size_t max_send_size, send_size;

    struct epoll_event ev, events[MAX_EVENTS];
    int nfds, epollfd;

    struct timespec wait_for_server={.tv_sec=0, .tv_nsec=INTERVAL};

    assert(sender_buf_size <= BUF_SIZE);

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

    max_send_size = MIN(MAX_UDP_SIZE, MIN(expected_size, sender_buf_size));
    *received = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // when performing throughput, sender needs not to care about receiver
    while (sent_size < expected_size) {
        send_size = MIN(max_send_size, expected_size - sent_size);
        Sendton(sockfd, buf, send_size, 0,
            (struct sockaddr *)&servaddr, sizeof(servaddr));
        if (DEBUG) printf("%u>>>>%u / %u sendto\n", send_size, sent_size, expected_size);
        sent_size += send_size;
    }
    if (DEBUG) printf("client finished throughput sent %u\n", sent_size);

    // the second while is used to receive multiple reply

    while (1) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, EPOLL_MILLI_TIMEOUT);
        if (nfds == -1) {
            perror("epoll_pwait");
            exit(EXIT_FAILURE);
        } else if (nfds == 0)
            break;

        for (i = 0; i < nfds; ++i) {
            if (events[i].events & EPOLLIN) { // readable
                while (1) {
                    // since server will respond to every sent packet, there may be multiple responds
                    ret = read(events[i].data.fd, &n, sizeof(int));
                    if (ret < 0) {
                        if (!((errno == EAGAIN) || (errno == EWOULDBLOCK)))
                            perror("read error other than nonblock read");
                        break;
                    } else if (ret == 0) // end of file
                        break;
                    else {
                        // single "ack" response contributes a small amount of time
                        clock_gettime(CLOCK_MONOTONIC, &end);
                        *received += n;
                        if (DEBUG) printf("<<<<%u echoed\n", n);
                    }
                }
                if (DEBUG) printf("%i / %u ack Received\n", *received, expected_size);
            }
        }
    }
    *latency = (end.tv_sec - start.tv_sec) * 1000000000 +
        end.tv_nsec - start.tv_nsec;
    close(sockfd);
}


#ifdef CLIENT_MAIN
int main(int argc, char**argv)
{
    struct sockaddr_in servaddr;
    unsigned chunk_size[] = {4, 16, 64, 256, 1024, 4*1024, 16*1024, 64*1024, 256*1024, 512*1024, 32*1024, 48*1024, 96*1024, 128*1024};
    //unsigned chunk_size[] = {4, 16, 64, 256, 1024, 4*1024, 16*1024, 32*1024, 64*1024, 96*1024, 128*1024};
    unsigned latency, total_time;
    size_t received;
    int i;
    int repeat=1;
    size_t sender_buf_size=1024;

    if (argc >= 5)
        sender_buf_size = atoi(argv[4]);
    if (argc >= 4)
        repeat = atoi(argv[3]);

    if (argc < 3) {
        printf("usage: [udp|tcp] <IP address> <repeat times> <sender_buf_size=1024>\n");
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
            printf("SIZE\tUDP latency us\tsender_buf_size: %i\n", sender_buf_size);
            for (i=0; i<sizeof(chunk_size) / sizeof(chunk_size[0]); i++) {
                call_udpl_epoll(servaddr, chunk_size[i], &latency, sender_buf_size);
                printf("%8u\t%8u\n", chunk_size[i], latency / 1000);
            }
        } else if (strcmp(argv[1], "udpt") == 0) {
            printf("SIZE\tUDP throughput MB/s\tLOST\tLOSS Rate%%\tsender_buf_size: %i\n", sender_buf_size);
            for (i=0; i<sizeof(chunk_size) / sizeof(chunk_size[0]); i++) {
                call_udpt_epoll(servaddr, chunk_size[i], &latency, sender_buf_size, &received);
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
