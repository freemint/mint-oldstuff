/*
 *	 this one is fixed code taken from the mintlib PL44, supplied here
 *	because it wouldn't work with the standard library function...
 */

/* access() emulation; relies heavily on stat() */

#include <types.h>
#include <stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

extern int __mint;

int
myaccess(path, mode)
	const char *path;
	int mode;
{
	int euid;
	struct stat sb;

	if (stat(path, &sb) < 0)
		return -1;	/* errno was set by stat() */
	if (mode == F_OK)
		return 0;	/* existence test succeeded */

	if ((euid = geteuid()) == 0) return 0; /* super user can access anything */

/* somewhat crufty code -- relies on R_OK, etc. matching the bits in the
   file mode, but what the heck, we can do this
 */
	if (__mint < 9 || ( euid == sb.st_uid ) ) {
		if ( ((sb.st_mode >> 6) & mode) == mode )
			return 0;
		else
			goto accdn;
	}

	if ( getegid() == sb.st_gid ) {
		if ( ((sb.st_mode >> 3) & mode) == mode )
			return 0;
		else
			goto accdn;
	}

	if ( (sb.st_mode & mode) == mode)
		return 0;
accdn:
	errno = EACCESS; return -1;
}
