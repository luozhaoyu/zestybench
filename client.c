#include "protocol.h"

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
    unsigned n;

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
    unsigned n;

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
    if (DEBUG) printf("expected_size: %u sent, require_echo: %i,  Reading...\n",
        data.expected_size, data.require_echo);
    if (data.require_echo) {
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


#ifdef CLIENT_MAIN
int main(int argc, char**argv)
{
    struct sockaddr_in servaddr;
    unsigned chunk_size[] = {4, 16, 64, 256, 1024, 4*1024, 16*1024, 64*1024,
        256*1024, 512*1024};
    unsigned latency, total_time;
    int received;
    int i;

    if (argc != 3)
    {
       printf("usage: [udp|tcp] <IP address>\n");
       exit(1);
    }

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(argv[2]);
    servaddr.sin_port=htons(PORT);

    if (strcmp(argv[1], "udp") == 0) {
        printf("SIZE\tUDP latency us\tUDP througput MB/s\tLOST\tLOSS rate%\n");
        for (i=6; i<10; i++) {
            call_udp(servaddr, 1, chunk_size[i], &latency, NULL);
            call_udp(servaddr, 0, chunk_size[i], &total_time, &received);
            printf("%8u\t%8u\t%8.2f\t%8u\t%8.2f\n", chunk_size[i], latency / 1000,
                chunk_size[i] * 1000000000.0 / total_time / 1024 / 1024, chunk_size[i] - received,
                (chunk_size[i] - received) * 100.0 / chunk_size[i]);
        }
    } else if (strcmp(argv[1], "tcp") == 0) {
        printf("SIZE\tTCP latency us\tTCP througput MB/s\tLOST\tLOSS rate\n");
        for (i=0; i<10; i++) {
            call_tcp(servaddr, 1, chunk_size[i], &latency, NULL);
            call_tcp(servaddr, 0, chunk_size[i], &total_time, &received);
            printf("%8u\t%8u\t%8.2f\t%8u\t%8.2f\n", chunk_size[i], latency / 1000,
                chunk_size[i] * 1000000000.0 / total_time / 1024 / 1024, chunk_size[i] - received,
                (chunk_size[i] - received) * 100.0 / chunk_size[i]);
        }
    }
}
#endif
