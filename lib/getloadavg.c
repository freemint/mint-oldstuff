/*
 *  getloadavg() -- Get system load averages.
 *
 *  Put `nelem' samples into `loadavg' array.
 *  Return number of samples retrieved, or -1 on error.
 *
 *  1.0		S.R.Usher 2nd August, 1994. (02/08/94)
 *
 *  Note: This will only work with a version of MiNT which includes my
 *		uptime/loadaverage patches.
 *
 * Placed into the public domain on 10th September, 1994.
 *
 */

#include <sys/errno.h>
#include <mintbind.h>

#ifndef Suptime
#define Suptime(uptime, avenrun)				\
		trap_1_wll(0x13f, (long)(uptime), (long)(avenrun))
#endif

#define LOAD_SCALE 2048.0

getloadavg(loadavg, nelem)
double loadavg[];
int nelem;
{
	register int i;
	unsigned long uptime;
	unsigned long avenrun[3];
	double realval;

	if (nelem <= 0)
	{
		errno = EINVAL;
		return -1;
	}
	
	if ((i = Suptime(&uptime, avenrun)) < 0)
	{
#ifndef ENOSYS		/* function not implemented (POSIX) */
		errno = -i;
#else
		errno = ENOSYS;
#endif

		return -1;
	}

	if (nelem > 3)
		nelem = 3;

	for (i = 0; i < nelem; i++)
	{
		realval = avenrun[i];
		loadavg[i] = realval / (double) LOAD_SCALE;
	}

	return nelem;
}

#ifdef TEST
main()
{
	double avgs[3];

	if (getloadavg(avgs, 3) < 0)
	{
		perror("getloadavg");
		exit(0);
	}

	printf("getloadavg returns %-5.2f %-5.2f %-5.2f\n", avgs[0], avgs[1], avgs[2]);
}
#endif
