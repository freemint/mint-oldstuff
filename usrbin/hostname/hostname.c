#include <stdio.h>

#define MAXHOSTLEN 256

main(argc, argv)
int argc;
char *argv[];
{
	char hostname[MAXHOSTLEN + 1];

	if (gethostname(hostname, MAXHOSTLEN) < 0)
		perror(argv[0]);
	else
		printf("%s\n", hostname);
}
