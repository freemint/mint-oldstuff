/*
 * write - send a message to another user
 * (C) 1991 D P Gymer
 * This code may be freely redistributed under the terms of the GNU GPL.
 *
 * heavily modified for MiNTOS 1.3 06/94 by
 * Torsten Scherer, itschere@techfak.uni-bielefeld.de
 *
 * $Log$
 */

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termcap.h>
#include <unistd.h>
#include <utmp.h>

const char *rcsid = "$Id$";

#define UTMP	"/etc/utmp"

static void
do_write(line)
    char *line;
{
    int fd,
	c;
    char host[16];
    FILE *fp;

    if (gethostname(host, 16))
	strcpy(host, "unknown-host");
    signal(SIGTTOU, SIG_IGN);
    if ((fd = open(line, O_WRONLY, 0)) < 0 || !(fp = fdopen(fd, "w"))) {
	perror(line);
	exit(1);
    }
    fprintf(fp, "\nMessage from %s@%s on %s...\n", getlogin(), host, ttyname(-1));
    while ((c = getchar()) != EOF)
	fputc(c, fp);
    fputs("EOF\n", fp);
    fclose(fp);
    signal(SIGTTOU, SIG_DFL);
}

char	linebuf[14];

char *findline(char *user)
{
	int		fd, count, found;
	struct utmp	utmp;
	char		writeto[9];

	if ((fd = open(UTMP, O_RDONLY, 0)) < 0) {
	    perror(UTMP);
	    exit(1);
	}

	count = 0;
	found = 0;
	while (read(fd, &utmp, sizeof(struct utmp)) == sizeof(struct utmp)) {
		if (*utmp.ut_line && !strncmp(utmp.ut_name, user, 8)) {
			count++;
			if (!found) {
				strncpy(writeto, utmp.ut_line, 8);
				writeto[8] = 0;
				sprintf(linebuf, "/dev/%s", writeto);
				if (!myaccess(linebuf, W_OK))
					found = 1;
			}
		}
	}

	close(fd);

	if (found) {
		if (count > 1)
			printf("user logged in more than once, trying %s...\n", writeto);
		return linebuf;
	}

	if (!count)
		fprintf(stderr, "Sorry, %s is not logged on!\n", user);
	else
		fprintf(stderr, "Sorry, none of %s's terminals can be written to!\n", user);

	return NULL;
}

char *verifyline(char *user, char *line)
{
	int		fd, found;
	struct utmp	utmp;

	if ((fd = open(UTMP, O_RDONLY, 0)) < 0) {
	    perror(UTMP);
	    exit(1);
	}

	found = 0;
	while (!found && (read(fd, &utmp, sizeof(struct utmp)) == sizeof(struct utmp))) {
		if (!strncmp(utmp.ut_line, line, 8)) {
			sprintf(linebuf, "/dev/%s", utmp.ut_line);
			if (!strncmp(utmp.ut_name, user, 8)) {
				if (myaccess(linebuf, W_OK)) {
					found = -1;
					printf("writes to that terminal are forbidden!\n");
				} else
					found = 1;
			} else {
				found = -1;
				printf("%s is not logged to that terminal!\n", user);
			}
		}
	}

	close(fd);

	if (found > 0) {
		return linebuf;
	}

	return NULL;
}

static void
usage()
{
    fprintf(stderr, "Usage: write [tty] user\n");
    exit(1);
}


/*
 *
 */

void main(int argc, char *argv[])
{
	char	*user, *line;

	switch (argc) {
		case 2: user = argv[1];
			line = NULL;
			break;

		case 3: user = argv[2];
			line = argv[1];
			break;

		default: usage();
	}

	if (line) {
		 if ((line = verifyline(user, line)))
			do_write(line);
	} else if ((line = findline(user)))
			do_write(line);
}
