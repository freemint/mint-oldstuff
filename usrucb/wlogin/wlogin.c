/*
 * wlogin - make a (typically MGR) window look like a login terminal
 * (C) 1992 Dave Gymer
 * $Log: wlogin.c,v $
 * Revision 0.1  1992/01/26  11:53:40  dpg
 * Initial revision.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <utmp.h>

static char rcsid[] = "$Id: wlogin.c,v 0.1 1992/01/26 11:53:40 dpg Exp $";

#define LOGIN "/usr/bin/login"

int
main(argc, argv)
	int argc;
	char **argv;
{
	char *term;

	if (argc > 1) {
		fprintf(stderr, "Usage: %s\n", *argv);
		return 1;
	}
	if (!(term = ttyname(0))) {
		perror("file handle 0 is not a tty");
		return -1;
	}
	if (!strncmp(term, "/dev/", 5))
		term += 5;
	else
		fprintf(stderr, "%s: warning, terminal not in /dev\n", *argv);

	switch (fork()) {
		case -1:
			perror(*argv);
			return 2;
		case 0:
			execl(LOGIN, LOGIN, (char *) 0);
			perror(LOGIN);
			return 2;
		default:
			wait((int *) 0);
	}
	write_utmp(term, "", "", time(0L));
	write_wtmp(term, "", "", time(0L));
	return 0;
}
