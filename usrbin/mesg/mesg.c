/*
 *	mesg.c	-	(C) 09/93 by Torsten Scherer
 *			itschere@techfak.uni-bielefeld.de (Internet)
 *			>>> see COPYING for more info on copyrights <<<
 *
 *	allows or forbids writing to the current terminal for other users,
 *	thus making write() or talk() impossible if you don't want them
 *
 *	BUG:	needs a version of MiNT with XATTR fields in the BiosFS
 *
 *	V1.0	21.Sep.1993	T.Scherer
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

	if ((st.st_mode & S_IWGRP) && (st.st_mode & S_IWOTH)) {
		printf("mesg: writes to this terminal are allowed\n");
		return;
	}

	printf("mesg: writes to this terminal are NOT allowed\n");
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
		if (chmod(name, 402) == 0)
			printf("mesg: writes to terminal now allowed\n");
		else
			fprintf(stderr, "error changing mesg mode to <yes>\n");
	} else {
		if (chmod(name, 384) == 0)
			printf("mesg: writes to terminal now forbidden\n");
		else
			fprintf(stderr, "error changing mesg mode to <no>\n");
	}
}

void main(int argc, char *argv[])
{
	switch(argc)
	{
		case 1:	show_mode(); break;
		case 2: change_mode(argv[1]); break;
		default: usage(); break;
	}
}


