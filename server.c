#include "protocol.h"

/**
 * struct sockaddr_in servaddr
 *
 * Descriptions
 **/
void
start_tcp(struct sockaddr_in servaddr)
{
    struct sockaddr_in cliaddr;
    int listenfd,connfd,n;
    socklen_t clilen;
    pid_t     childpid;
    char buf[BUF_SIZE];
    unsigned expected_size;
    bool require_echo;
    int optval=1;

    listenfd=socket(AF_INET,SOCK_STREAM,0);
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0)
        perror("setsockopt SO_REUSEADDR");
    if (setsockopt(listenfd, IPPROTO_IP, TCP_NODELAY, &optval, sizeof(optval)) != 0)
        perror("setsockopt TCP_NODELAY");
    if (bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) != 0) perror("bind");
    if (listen(listenfd,1024) != 0) perror("listen");
    printf("tcp server at: %d\n", PORT);
    while (1) {
        clilen=sizeof(cliaddr);
        connfd = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);
        childpid = fork();
        if (childpid < 0)
            perror("fork");
        else if (childpid == 0) {
            // this is child
            close(listenfd);
            if (Readn(connfd, &expected_size, sizeof(unsigned)) < sizeof(unsigned))
                printf("Failed to receive expected_size: %u?\n", expected_size);
            if (Readn(connfd, &require_echo, sizeof(bool)) < sizeof(bool))
                printf("Failed to receive require_echo: %i?\n", require_echo);
            n = Readn(connfd, buf, expected_size);
            if (require_echo) {
                Writen(connfd, buf, expected_size);
            } else {
                Writen(connfd, &n, sizeof(n));
            }
            close(connfd);
            _exit(EXIT_SUCCESS);
        } else {
            // this is parent
            close(connfd);
        }
    }
}


/**
 * int active_sock: 
 *
 * Descriptions
 **/
void
udp_process(struct SimpleState *state)
{
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    char buf[BUF_SIZE];
    int n;

    if (state->protocol.expected_size) {
        // second read, read as many data as possible
        clilen=sizeof(cliaddr);
        if (DEBUG) printf("Header received: expected_size=%u, require_echo=%i, waiting data...\n",
            state->protocol.expected_size, state->protocol.require_echo);
        n = Recvfromn(state->fd, buf, state->protocol.expected_size, 0,
            (struct sockaddr *)&cliaddr, &clilen);
        if (n < state->protocol.expected_size)
            printf("WARNING\tonly %i of %u data received\n", n, state->protocol.expected_size);
        if (DEBUG) printf("%i of %u received, require_echo=%i. Responding...\n",
            n, state->protocol.expected_size, state->protocol.require_echo);
        if (state->protocol.require_echo) {
            Sendton(state->fd, buf, state->protocol.expected_size, 0,
                (struct sockaddr *)&cliaddr, clilen);
            if (DEBUG) printf("Echoed\n");
        } else {
            Sendton(state->fd, &n, sizeof(int), 0,
                (struct sockaddr *)&cliaddr, clilen);
            if (DEBUG) printf("No echo, %i of %u received\n",n, state->protocol.expected_size);
        }
        // clear protocol to go back to the first phase
        bzero(&(state->protocol), sizeof(struct SimpleProtocol));
    } else {
        // first connect, read header first
        if (Readn(state->fd, &state->protocol, sizeof(struct SimpleProtocol)) < sizeof(struct SimpleProtocol))
            printf("Failed to receive protocol header data expected_size: %u, require_echo: %i?\n",
                state->protocol.expected_size, state->protocol.require_echo);
        if ((state->protocol.require_echo != 0 && state->protocol.require_echo != 1)
            || (state->protocol.expected_size > BUF_SIZE)) {
            printf("WARNING\tprotocol header maybe broken, expected_size: %u, require_echo: %i\t"
                "server decides to adopt maximum buffer size without echo\n",
                state->protocol.expected_size, state->protocol.require_echo);
            state->protocol.expected_size = BUF_SIZE;
            state->protocol.require_echo = 0;
        }
    }
}


/**
 * struct sockaddr_in servaddr
 *
 * Descriptions
 **/
void
start_udp(struct sockaddr_in servaddr)
{
    int n;
    struct epoll_event ev, events[MAX_EVENTS];
    int listen_sock, nfds, epollfd;

    struct SimpleState state;
    struct SimpleProtocol protocol;

    listen_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (setNonblocking(listen_sock) != 0) perror("setNonblocking");
    bind(listen_sock,(struct sockaddr *)&servaddr,sizeof(servaddr));

    printf("udp server at: %d\n", PORT);

    epollfd = epoll_create(16);
    if (epollfd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    //ev.data.fd = listen_sock;
    bzero(&state, sizeof(state));
    bzero(&protocol, sizeof(protocol));
    state.fd = listen_sock;
    state.protocol = protocol;
    ev.data.ptr = &state;
    // need not care about blocking
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_pwait");
            exit(EXIT_FAILURE);
        }

        for (n = 0; n < nfds; ++n) {
            if (events[n].events & EPOLLIN) { // readable
                udp_process((struct SimpleState *)events[n].data.ptr);
            }
        }
    }
    close(listen_sock);
}


void
start_udp_epoll(struct sockaddr_in servaddr, bool is_test_latency)
{
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    char buf[BUF_SIZE];
    int n, i;
    int listen_sock;

    struct epoll_event ev, events[MAX_EVENTS];
    int nfds, epollfd;

    listen_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (setNonblocking(listen_sock) != 0) perror("setNonblocking");
    bind(listen_sock,(struct sockaddr *)&servaddr,sizeof(servaddr));
    clilen = sizeof(cliaddr);

    printf("udp server at: %d, is_test_latency: %i\n", PORT, is_test_latency);

    epollfd = epoll_create(16);
    if (epollfd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listen_sock;
    // need not care about blocking
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_pwait");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < nfds; ++i) {
            if (events[i].events & EPOLLIN) { // readable
                //n = Recvfromn(events[i].data.fd, buf, BUF_SIZE, 0,
                //    (struct sockaddr *)&cliaddr, &clilen);
                n = recvfrom(events[i].data.fd, buf, BUF_SIZE, 0,
                    (struct sockaddr *)&cliaddr, &clilen);
                if (DEBUG) printf("Received %i\n", n);
                if (is_test_latency)
                    Sendton(events[i].data.fd, buf, n, 0,
                        (struct sockaddr *)&cliaddr, clilen);
                else
                    Sendton(events[i].data.fd, &n, sizeof(n), 0,
                        (struct sockaddr *)&cliaddr, clilen);
                if (DEBUG) printf("Finish %i\n", n);
            }
        }
    }
    close(listen_sock);
}


void
start_udp_throughput(struct sockaddr_in servaddr)
{
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    char buf[BUF_SIZE];
    int n;
    int listen_sock;

    listen_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (setNonblocking(listen_sock) != 0) perror("setNonblocking");
    bind(listen_sock,(struct sockaddr *)&servaddr,sizeof(servaddr));

    printf("udp throughput server at: %d\n", PORT);
    while (1) {
        // this should be Nonblocking to avoid block
        n = Recvfromn(listen_sock, buf, BUF_SIZE, 0,
            (struct sockaddr *)&cliaddr, &clilen);
        Sendton(listen_sock, &n, sizeof(n), 0,
            (struct sockaddr *)&cliaddr, clilen);
        if (DEBUG) printf("Finish ack %i message\n", n);
    }
    close(listen_sock);
}


#ifdef SERVER_MAIN
int main(int argc, char**argv)
{
    struct sockaddr_in servaddr,cliaddr;
    if (argc != 2)
    {
       printf("usage: [udp|tcp]\n");
       exit(1);
    }

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(PORT);

    if (strcmp(argv[1], "udpl") == 0) {
        start_udp_epoll(servaddr, 1);
    } else if (strcmp(argv[1], "udpt") == 0) {
        start_udp_epoll(servaddr, 0);
    } else if (strcmp(argv[1], "udp") == 0) {
        start_udp(servaddr);
    } else if (strcmp(argv[1], "tcp") == 0) {
        start_tcp(servaddr);
    }
}
#endif
