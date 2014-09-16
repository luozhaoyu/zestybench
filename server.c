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

    listenfd=socket(AF_INET,SOCK_STREAM,0);
    bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    listen(listenfd,1024);
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
 * struct sockaddr_in servaddr
 *
 * Descriptions
 **/
void
start_udp(struct sockaddr_in servaddr)
{
    struct sockaddr_in cliaddr;
    int listenfd, n;
    socklen_t clilen;
    pid_t     childpid;
    char buf[BUF_SIZE];
    struct SimpleProtocol data;

    listenfd = socket(AF_INET, SOCK_DGRAM, 0);
    bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    printf("udp server at: %d\n", PORT);
    while (1) {
        clilen=sizeof(cliaddr);
        if (Recvfromn(listenfd, &data, sizeof(data), 0,
            (struct sockaddr *)&cliaddr, &clilen) < sizeof(data))
            printf("Failed to receive protocol header data expected_size: %u, require_echo: %i?\n",
                data.expected_size, data.require_echo);
        if (DEBUG) printf("Header received expected_size: %u, require_echo: %i, waiting data...\n",
            data.expected_size, data.require_echo);
        n = Recvfromn(listenfd, buf, data.expected_size, 0,
            (struct sockaddr *)&cliaddr, &clilen);
        if (n < data.expected_size)
            printf("Failed to receive buf: %i bytes?\n", n);
        if (DEBUG) printf("%i of %u received, %i require_echo. Sending...\n",
            n, data.expected_size, data.require_echo);
        if (data.require_echo) {
            Sendton(listenfd, buf, data.expected_size, 0,
                (struct sockaddr *)&cliaddr, clilen);
        } else {
            Sendton(listenfd, &n, sizeof(int), 0,
                (struct sockaddr *)&cliaddr, clilen);
        }
        printf("finish one response, waiting new...\n");
    }
    close(listenfd);
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

    if (strcmp(argv[1], "udp") == 0) {
        start_udp(servaddr);
    } else if (strcmp(argv[1], "tcp") == 0) {
        start_tcp(servaddr);
    }
}
#endif
