/*
 *	talk - an online chat program for MiNT
 *
 *	doesn't use multiple windows so far :-(
 */

#include <stdio.h>
#include <utmp.h>
#include <string.h>
#include <mintbind.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <curses.h>	/* CAUTION */

#define UTMP "/etc/utmp"

int get_user_id(char *uname)
{
	FILE	*fp;
	int	lauf, lauf2, ret = -1;
	char	buff[256], compare[9], a_uid[6];

	fp = fopen("/etc/passwd", "r");

	while ( ( ret == -1 ) && ( !feof(fp) ) )
	{
		fgets(buff, 256, fp);

		/* username */
		lauf = 0;
		while ( buff[lauf] != ':')
			lauf++;
		strncpy(compare, buff, lauf);
		compare[lauf] = 0;

		if (!strcmp(uname, compare)) {

			/* password */
			lauf++;
			while ( buff[lauf] != ':')
				lauf++;

			/* uid */
			lauf++;
			lauf2 = lauf;
			while ( buff[lauf] != ':')
				lauf++;
			strncpy(a_uid, &buff[lauf2], lauf - lauf2);
			a_uid[lauf-lauf2] = 0;

			ret = atol(a_uid);
		}
	}
	fclose(fp);

	return ret;
}

static char *findline(char *user, char *line)
{
	static char linebuf[14];
	char buf2[9];
	int fd;
	struct utmp utmp;

	if (!line) {

		/* try to get the terminal line */

		if ((fd = open(UTMP, O_RDONLY, 0)) < 0) {
			perror(UTMP);
			exit(1);
		}

		while (read(fd, &utmp, sizeof(struct utmp)) == sizeof(struct utmp)) {
			if (!strncmp(utmp.ut_name, user, 8)) {	/* Magic #! */
				/* found user */
				if (line) {
					printf("User logged in more than once; trying %s...\n", line);
				} else {
					line = buf2;
					strncpy(buf2, utmp.ut_line, 8);
					buf2[8] = '\0';
				}
			}
		}
		close(fd);
	} else {

		/* just check if the given one is correct */

		line = 0;

		if ((fd = open(UTMP, O_RDONLY, 0)) < 0) {
			perror(UTMP);
			exit(1);
		}

		while (read(fd, &utmp, sizeof(struct utmp)) == sizeof(struct utmp)) {
			if (!strncmp(utmp.ut_line, line, 8)) {
				/* found line */
				if (!strncmp(utmp.ut_name, user, 8)) {
					line = buf2;
					strncpy(buf2, utmp.ut_line, 8);
					buf2[8] = '\0';
				}
			}
		}
		if (!line) {
			fprintf(stderr, "%s is not logged to that terminal!\n", user);
			exit(1);
		}
		close(fd);
	}

	if (line) {
		sprintf(linebuf, "/dev/%s", line);
		line = linebuf;
	}

	return line;
}

void send_message(char *line)
{
	char	host[16];
	char	tosline[14], *ptr;
	char	buf[256];
	int	lineh;

	if (gethostname(host, 16))
		strcpy(host, "unknown-host");

	printf("ringing your party\n");

	signal(SIGTTOU, SIG_IGN);

	/*
	 * convert filename to TOS format, cause we're using system calls
	 * instead of library calls, they always seem to open the file
	 * for reading also, and this will not work if the terminal isn't
	 * set up for reading from
	 */

	strcpy(tosline, line);
	ptr = tosline;
	while (*ptr) {
		if (*ptr == '/')
			*ptr = '\\';
		ptr++;
	}

	if ((lineh = Fopen(tosline, 1)) < 0) {
		fprintf(stderr, "sorry, can't write to terminal %s\n", line);
		exit(-1);
	}

	/* print the message */
	sprintf(buf, "\n\r\rSystem Message: talk requested by %s@%s on %s\n\rrespond with: talk %s\n\r\r", getlogin(), host, ttyname(-1), getlogin());
	Fwrite(lineh, strlen(buf), buf);

	Fclose(lineh);
	signal(SIGTTOU, SIG_DFL);
}

void usage()
{
	fprintf(stderr, "Usage: talk [tty] user\n");
	exit(-1);
}

int connection(int pipehandle)
{
	char	c;
	int	numin, numout;

	clear();

	for (;;) {

		/* check for received characters */
		if ((numin = Finstat(pipehandle)) > 0) {
			Fread(pipehandle, 1, &c);
			standout();
			Fwrite(1, 1, &c);
			standend();
			if (c == 4)
				return 2;
		}

		/* check for characters to be send */
		if ((numout = Finstat(0)) > 0) {
			Fread(0, 1, &c);
			Fwrite(pipehandle, 1, &c);

			if (c == 4)
				return 1;
		}

		/* there wasn't anything? be gentle... */
		if ((numin != 0) && (numout != 0))
			(void) Syield();
	}
}

void main(int argc, char *argv[])
{
	char	*user, *line, pipename[15];
	int	from_uid, to_uid, sleepcount;
	int	pipehandle;

	/* parse arguments */
	switch (argc) {

		case 2:		line = 0;
				user = argv[1];
				break;

		case 3:		line = argv[1];
				user = argv[2];
				break;

		default:	usage();
				break;
	}

	/* find target terminal line */
	if (!(line = findline(user, line))) {
		fprintf(stderr, "%s is not logged on!\n", user);
		exit(-1);
	}

	/* now we've got the complete information */
	from_uid = getuid();
	to_uid = get_user_id(user);	/* since user is logged on, it must exist */

	/* first check if this is meant to be a reply */
	sprintf(pipename, "\\pipe\\t%03i_%03i", to_uid, from_uid);
	if ((pipehandle = Fopen(pipename, 2)) > 0) {

		/* it is a reply to an existing call, now send a */
		/* random(?) byte to show we're ready to answer */
		Fwrite(pipehandle, 1, pipename);

	} else {

		/* not, so we've got to ring the user */
		sprintf(pipename, "\\pipe\\t%03i_%03i", from_uid, to_uid);
		if ((pipehandle = Fcreate(pipename, 0)) < 0) {
			fprintf(stderr, "too many connections to remote user\n");
			exit(-1);
		}

		/* now _do_ ring him */
		sleepcount = 0;
		while ((Finstat(pipehandle)) == 0) {
			if (sleepcount == 0) send_message(line);
			sleep(1);
			sleepcount++;
			if (sleepcount == 10) sleepcount = 0;
		}

		/* read the confirmation byte from the pipe, this */
		/* trashes the pipename, but then: who cares??? */
		Fread(pipehandle, 1, pipename);
	}

	/* the pipe is now open and ready for conversation */
	printf("connection established\n");

	initscr();

	if ((connection(pipehandle)) == 1)
		printf("\n\nconnection closed\n\n");
	else
		printf("\n\nconnection closed by remote user\n\n");

	endwin();

	/* what's remaining? */
	Fclose(pipehandle);
}
