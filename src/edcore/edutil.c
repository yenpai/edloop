#include "edbase.h"

void edutil_time_curr(struct timeval * tv)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    tv->tv_sec  = ts.tv_sec;
    tv->tv_usec = ts.tv_nsec / 1000;
}

void edutil_time_next(struct timeval * tv, int msec)
{
    edutil_time_curr(tv);

    if (msec > 0)
    {
        tv->tv_sec  += (msec / 1000);
        tv->tv_usec += (msec % 1000) * 1000;

        while (tv->tv_usec > 1000000)
        {
            tv->tv_sec++;
            tv->tv_usec -= 1000000;
        }
    }
}

int edutil_time_diff(struct timeval * t1, struct timeval * t2)
{
    return (t1->tv_sec - t2->tv_sec) * 1000 +
        (t1->tv_usec - t2->tv_usec) / 1000 ;
}

