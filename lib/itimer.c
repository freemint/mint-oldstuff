#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include "itimer.h"

static struct itimerval it;
static void (*oldfunc) (int);


static void
alarm_handler (sig)
	int sig;
{
	if (it.it_interval.tv_sec > 0)
		alarm (it.it_interval.tv_sec);

	(*oldfunc) (sig);
}

int
setitimer (which, new, old)
	int which;
	struct itimerval *new;
	struct itimerval *old;
{
	int r;

	if (which != ITIMER_REAL) {
		errno = EINVAL;
		return -1;
	}

	it = *new;
	if (old) *old = it;

	oldfunc = signal (SIGALRM, alarm_handler);
	if ((long)oldfunc == -1L) {
		errno = EINVAL;
		return -1;
	}

	alarm (it.it_value.tv_sec);
	return 0;
}

int
getitimer (which, old)
	int which;
	struct itimerval *old;
{
	if (which != ITIMER_REAL) {
		errno = EINVAL;
		return -1;
	}
	*old = it;
	return 0;		
}
