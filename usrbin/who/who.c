/*
 * Who.c  A re-implementation of the BSD who(1) command.
 *
 *	Version 1.0 (c) S.R.Usher 1991.
 *
 * Changelog
 *
 * 20/10/91	S.R.Usher	1.0	First version.
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <utmp.h>

#define UTMP_FILE "/etc/utmp"

main()
{
	int fd;
	struct utmp entry;

	if ((fd = open(UTMP_FILE, O_RDONLY)) == -1)
	{
		perror(UTMP_FILE);
		exit(0);
	}

	while (read(fd, &entry, sizeof(struct utmp)) == sizeof(struct utmp))
	{
		if (!nonuser(entry))
		{
			if (entry.ut_name[0] != '\0')
			{
				print_entry(&entry);
			}
		}
	}

	close(fd);
	exit(0);
}

print_entry(entry)
struct utmp *entry;
{
	char *fmt;
	char output[80];
	struct tm *this_time;

	static char *dow[7] = {
		"Sun",
		"Mon",
		"Tue",
		"Wed",
		"Thu",
		"Fri",
		"Sat"
	};

	static char *moy[12] = {
		"Jan",
		"Feb",
		"Mar",
		"Apr",
		"May",
		"Jun",
		"Jul",
		"Aug",
		"Sep",
		"Oct",
		"Nov",
		"Dec"
	};


	if (entry->ut_host[0] != '\0')
		fmt = "%-8s %-8s %s %s %2d %2d:%02d:%02d (%s)\n";
	else
		fmt = "%-8s %-8s %s %s %2d %2d:%02d:%02d\n";

	this_time = localtime(&(entry->ut_time));

	printf(fmt, entry->ut_name, entry->ut_line,
			dow[this_time->tm_wday],
			moy[this_time->tm_mon],
			this_time->tm_mday,
			this_time->tm_hour,
			this_time->tm_min,
			this_time->tm_sec,
			entry->ut_host);
}
