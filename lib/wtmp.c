/*
 * BSD style wtmp updating routine Version 1.0 (c) S.R.Usher 1991.
 *
 * Note: this is a MiNTOS utility routine, not a standard UNIX routine.
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <utmp.h>
#include <errno.h>

extern char *sys_errlist[];

#define WTMP_FILE	"/var/adm/wtmp"

write_wtmp(line, name, host, time)
char *line;
char *name;
char *host;
unsigned long time;
{
	register int returned_val;
	int fd;
	int counter = 0;
	struct utmp entry;

	if ((fd = open(WTMP_FILE, (O_RDWR | O_APPEND))) == -1)
	{
		printf("write_wtmp: %s\n", sys_errlist[errno]);
		return -1;
	}

/*
 * Note, doing this in this order means that it doesn't matter about the Null
 * bytes strncpy adds the the strings if they are greater than 8/16 bytes!
 */

	strncpy(entry.ut_line, line, 8);
	strncpy(entry.ut_name, name, 8);
	strncpy(entry.ut_host, host, 16);
	entry.ut_time = time;

	if (write(fd, &entry, sizeof(struct utmp)) == -1)
	{
		printf("write_wtmp: %s\n", sys_errlist[errno]);
		return -1;
	}

	close(fd);
}
