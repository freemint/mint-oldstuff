#include <stdio.h>

#define MAXSHELLLEN 	80
#define SHELLSFILE	"/etc/shells"

static char buffer[MAXSHELLLEN];

#define NUMDEFSHELLS 4

static char *default_shells[] = {
	"/bin/sh",
	"/bin/csh",
	"/usr/bin/sh",
	"/usr/bin/csh"
};

static int opened = 0;
static int using_defs = 0;
static int cur_def_shell = 0;
static FILE *fp;

void setusershell()
{
	if (!opened)
		if ((fp = fopen(SHELLSFILE, "r")) == NULL)
			using_defs = 1;

	opened = 1;

	if (!using_defs)
		rewind(fp);
}

char *getusershell()
{
	if (!opened)
		setusershell();

	if (using_defs)
	{
		if (cur_def_shell < NUMDEFSHELLS)
		{
			strcpy(buffer, default_shells[cur_def_shell++]);
			return buffer;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if (fgets(buffer, MAXSHELLLEN, fp) != NULL)
		{
			buffer[strlen(buffer) - 1] = '\0';
			return buffer;
		}
		else
			return NULL;
	}
}

void endusershell()
{
	fclose(fp);
	cur_def_shell = using_defs = opened = 0;
}

#ifdef TEST
main()
{
	char *bink;
	
	for(;(bink = getusershell()) != NULL; printf("%s\n", bink));
}
#endif
