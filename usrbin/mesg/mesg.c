/*
 *	mesg.c - (C) 1993-94 by TeSche (Torsten Scherer),
 *	itschere@techfak.uni-bielefeld.de
 *	may be freely redistributed under the terms of the GNU-GPL
 *
 *	allows or forbids writing to the current terminal for other users,
 *	thus making write() or talk() impossible if you don't want them
 *
 *	BUG:	needs at least MiNT 1.10
 *
 *	V1.0	21.Sep.1993	T.Scherer
 *	V1.1	19.Jun.1994	T.Scherer, looks a bit more like on SUN-OS now
 */

#include <stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

char *getttyname()
{
	char	*name;

	if ((name = ttyname(-1)) == 0) {
		fprintf(stderr, "Error: Can't resolve terminal name!\n");
		exit(-1);
	}

	if (access(name, F_OK)) {
			fprintf(stderr, "Error: Can't access terminal %s!\n", name);
		exit(-1);
	}

	return name;
}

void usage()
{
	printf("usage: mesg {y|n}\n");
	exit(-1);
}

void show_mode()
{
	char		*name;
	struct stat	st;

	name = getttyname();

	if (stat(name, &st)) {
		fprintf(stderr, "Error: can't stat terminal <%s>!\n", name);
		exit(-1);
	}

	if (((st.st_mode & S_IFMT) != S_IFCHR) && (st.st_mode & S_IFMT) != S_IFIFO) {
		fprintf(stderr, "Error: <%s> is not a terminal!\n", name);
		exit(-1);
	}

	if (st.st_mode & S_IWGRP) {
		printf("is y\n");
		return;
	}

	printf("is n\n");
}

void change_mode(char *s)
{
	char	*name;
	int	flag = 0;

	if (!stricmp(s, "y"))
		flag = 1;
	else if (!stricmp(s, "n"))
		flag = 0;
	else
		usage();

	name = getttyname();

	if (flag) {
		if (chmod(name, 0620)) {
			fprintf(stderr, "error changing mesg mode to `yes'\n");
		}
	} else {
		if (chmod(name, 0600)) {
			fprintf(stderr, "error changing mesg mode to `no'\n");
		}
	}
}

void main(int argc, char *argv[])
{
	switch(argc) {

		case 1:	show_mode();
			break;

		case 2: change_mode(argv[1]);
			break;

		default: usage();
			break;
	}
}
