#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/wait.h>
#include <string.h>

#include <stdbool.h>

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

int main()
{
    //get_resolution_of_clock_gettime();
    unsigned chunk_size[10] = {4, 16, 64, 256, 1024, 4*1024, 16*1024, 64*1024,
        256*1024, 512*1024};
    long unsigned t;
    int i;

    for (i=0; i<10; i++) {
        t = pipe_latency(chunk_size[i], 0);
        printf("PIPE latency\t%u:\t%.2fus\n", chunk_size[i], t / 1000.0);
        t = pipe_throughput(chunk_size[i], 0);
        printf("PIPE throughput\t%u:\t%.2fKB/s\n", chunk_size[i], chunk_size[i] * 1000000000.0 / t / 1024);
    }
    return 0;
}
