#include "edbase.h"

/* Parameters used to convert the timespec values: */
#define MSEC_PER_SEC    1000L
#define USEC_PER_MSEC   1000L
#define NSEC_PER_USEC   1000L
#define NSEC_PER_MSEC   1000000L
#define USEC_PER_SEC    1000000L
#define NSEC_PER_SEC    1000000000L
#define FSEC_PER_SEC    1000000000000000LL

void edutil_time_curr(struct timeval * tv)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    tv->tv_sec  = ts.tv_sec;
    tv->tv_usec = ts.tv_nsec / NSEC_PER_USEC;
}

void edutil_time_next(struct timeval * tv, unsigned int msec)
{
    edutil_time_curr(tv);

    if (msec > 0)
    {
        tv->tv_sec  += (msec / MSEC_PER_SEC);
        tv->tv_usec += (msec % MSEC_PER_SEC) * USEC_PER_MSEC;

        while (tv->tv_usec > USEC_PER_SEC)
        {
            tv->tv_sec++;
            tv->tv_usec -= USEC_PER_SEC;
        }
    }
}

long edutil_time_diff(struct timeval * t1, struct timeval * t2)
{
    return (t1->tv_sec - t2->tv_sec) * MSEC_PER_SEC +
        (t1->tv_usec - t2->tv_usec) / USEC_PER_MSEC ;
}

