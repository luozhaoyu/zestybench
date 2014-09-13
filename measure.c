#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

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

    if (clock_getres(CLOCK_MONOTONIC_RAW, &res) != 0) {
        printf("clock_getres: tv_sec=%ld, tv_nsec=%ld\n", res.tv_sec, res.tv_nsec);
    } else if (errno == EFAULT) {
        printf("EFAULT error\n");
    } else if (errno == EINVAL) {
        printf("EINVAL error\n");
    } else if (errno == EPERM) {
        printf("EPERM error\n");
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

}


int main()
{
    get_resolution_of_clock_gettime();
    return 0;
}
