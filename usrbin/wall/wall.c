/*
 *	wall.c - (C) 06/94 by TeSche (itschere@techfak.uni-bielefeld.de)
 *	may be freely redistributed under the terms of the GNU-GPL
 *
 *	V0.1 for MiNTOS 1.3
 */

#include <utmp.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#define	PATH_UTMP	"/etc/utmp"

FILE *read_message(char *fname)
{
	char	*hostname;
	FILE	*fp;
	long	t;
	char	buf[1024];

	if (!(hostname = getenv("HOST")))
		hostname = "unknown";

	if ((fp = fopen(fname, "w+"))) {
		t = time(0);
		fprintf(fp, "\n\nMessage from: %s@%s at %s", getlogin(), hostname, ctime(&t));
		while (fgets(buf, 1023, stdin))
			fputs(buf, fp);
		fprintf(fp, "EOF\n\n");
	}

	return fp;
}

void write_msg(FILE *fp, char *ttyname)
{
	char	ttypath[16];
	FILE	*tty;
	char	buf[1024];

	sprintf(ttypath, "/dev/%s", ttyname);
	if ((tty = fopen(ttypath, "w"))) {
		rewind(fp);
		while (fgets(buf, 1023, fp))
			fputs(buf, tty);
		fclose(tty);
	}
}

void main()
{
	char		*fname;
	FILE		*fp;
	short		fh;
	struct utmp	ut;

	if ((fp = read_message(fname = tmpnam(NULL)))) {

		if ((fh = open(PATH_UTMP, O_RDONLY)) < 0) {

			fprintf(stderr, "error: can't find utmp file!\n");
			exit(-1);

		} else {

			while (read(fh, &ut, sizeof(struct utmp)) == sizeof(struct utmp))
				if (*ut.ut_name)
					write_msg(fp, ut.ut_line);

			close(fh);
		}

		fclose(fp);
		unlink(fname);
	}
}
