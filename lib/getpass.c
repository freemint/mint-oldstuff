/*
 * getpass.c	Modified 02/05/94 S.R.Usher
 *
 * Although this is based upon the getpass.c in the normal MiNTlibs it is
 * somewhat different in that it is bug-for-bug compatable with the SunOS
 * 4.1.x version.
 */

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#define INPUTBUFLEN 1024

char *getpass(prompt)
const char *prompt;
{
	static char retbuf[9];
	static char inputbuf[INPUTBUFLEN];
	struct sgttyb orig, noecho;
	FILE *tty;
	int ttyfd;
	int c, i;
	int usingstdin = 0;

	fflush(stdin);
	fflush(stdout);

	if ((tty = fopen("/dev/tty", "r")) == NULL)
#ifdef SYSV
		return NULL;
#else
	{
		tty = stdin;
		usingstdin++;
	}
#endif

	ttyfd = fileno(tty);

	ioctl(ttyfd, TIOCGETP, &orig);
	ioctl(ttyfd, TIOCGETP, &noecho);

	noecho.sg_flags &= ~ECHO;
	
	ioctl(ttyfd, TIOCSETP, &noecho);
	
	printf(prompt);
	fflush(stdout);

	for (i = 0; i <= INPUTBUFLEN; inputbuf[i++] = '\0');

	for (i = 0; (i < INPUTBUFLEN) && (((c = fgetc(tty)) != EOF) && (c != '\n')) ;)
	{
		if ((c == '\010') || (c == '\177'))
		{
			if (i > 0)
				inputbuf[--i] = '\0';
		}
		else
			inputbuf[i++] = (char) c;
	}
	
	ioctl(ttyfd, TIOCSETP, &orig);

	printf("\n");
	fflush(stdout);

	if (!usingstdin)
		fclose(tty);

	for (i = 0; i < 8; i++)
		retbuf[i] = inputbuf[i];

	retbuf[8] = '\0';

	return retbuf;
}
