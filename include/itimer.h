#ifndef _ITIMER_H
#define _ITIMER_H

#include <sys/time.h>

#define ITIMER_REAL	0
#define ITIMER_PROF	1
#define ITIMER_VIRTUAL	2

struct itimerval {
	struct timeval	it_value;
	struct timeval	it_interval;
};

#endif
