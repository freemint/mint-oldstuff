/*
 * write - send a message to another user
 * (C) 1991 D P Gymer
 * This code may be freely redistributed under the terms of the GNU GPL.
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
    fprintf(fp, "Message from %s@%s on %s...\n", getlogin(), host, ttyname(-1));
    while ((c = getchar()) != EOF)
	fputc(c, fp);
    fputs("EOF\n", fp);
    fclose(fp);
    signal(SIGTTOU, SIG_DFL);
}

static char *
findline(user, line)
    char *user;
    char *line;
{
    static char buf1[14];	/* Magic number alert! */
    char buf2[9];
    int fd;
    struct utmp utmp;

    if (!line) {
	if ((fd = open(UTMP, O_RDONLY, 0)) < 0) {
	    perror(UTMP);
	    exit(1);
	}
	while (read(fd, &utmp, sizeof(struct utmp)) == sizeof(struct utmp)) {
	    if (!strncmp(utmp.ut_name, user, 8)) {	/* Magic #! */
		if (line) {
		    printf("User logged in more than once; trying %s...\n",
			line);
		} else {
		    line = buf2;
		    strncpy(buf2, utmp.ut_line, 8);
		    buf2[8] = '\0';
		}
	    } else if (!strncmp(utmp.ut_line, line, 8)) {
		fprintf(stderr, "%s is not logged to that terminal!\n", user);
		exit(1);
	    }
	}
	close(fd);
    }
    if (line) {
	sprintf(buf1, "/dev/%s", line);
	line = buf1;
    }
    return line;
}

static void
usage()
{
    fprintf(stderr, "Usage: write [tty] user\n");
    exit(1);
}

int
main(argc, argv)
    int argc;
    char **argv;
{
    int c;
    char *user,
       *line;
    extern int opterr,
        optind;

    opterr = 0;
    while ((c = getopt(argc, argv, "")) != EOF)
	switch (c) {
	default:
	    usage();
	}

    if (optind == argc - 2)
	line = argv[optind++];
    else
	line = 0;

    if (optind != argc - 1)
	usage();

    if (!(line = findline((user = argv[optind]), line))) {
	fprintf(stderr, "%s is not logged on!\n", user);
	exit(1);
    }

    do_write(line);

    return 0;
}
