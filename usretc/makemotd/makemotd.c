#include <stdio.h>

#include "version.h"

#define MOTD "/etc/motd"
#define TMPFILE "/tmp/motdtmp"
#define BUFSIZE 256

main()
{
	FILE *inf, *ouf;
	char buffer[BUFSIZE];

	_binmode(1);

	if ((ouf = fopen(TMPFILE, "w")) == NULL)
		exit(0);

	fprintf(ouf, "%s\n", MINTOSVERS);

	if ((inf = fopen(MOTD, "r")) != NULL)
	{
		fgets(buffer, BUFSIZE, inf);

		while (fgets(buffer, BUFSIZE, inf) != NULL)
			fprintf(ouf, buffer);

		fclose(inf);
	}

	fclose(ouf);

	if ((ouf = fopen(MOTD, "w")) == NULL)
		exit(0);

	if ((inf = fopen(TMPFILE, "r")) != NULL)
	{
		while (fgets(buffer, BUFSIZE, inf) != NULL)
			fprintf(ouf, buffer);

		fclose(inf);
	}

	fclose(ouf);

	unlink(TMPFILE);

	exit(0);
}
