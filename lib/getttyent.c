/*
 * Getttyent.c	A routine to parse a ttytab file and return a structure
 * 			filled with the information extracted.
 *
 * Version 1.1 (c) S.R.Usher 1992.
 *
 * Changelog:-
 *
 * Date		Version		Programmer	Comments
 * ----		-------		----------	--------
 * 21/9/91	1.0		S.R.Usher	First version
 *  9/1/92	1.1		S.R.Usher	Allow comment lines starting
 *							with a #.
 *
 * Placed into the public domain on 10th September, 1994.
 *
 */
 
#include <stdio.h>
#include <ctype.h>
#include <ttyent.h>

static FILE *ttyfile;
static char tty_name[32], tty_getty[256], tty_type[32], tty_window[256];
static char tty_comment[256], hmm[256];

static struct ttyent entry = {
	tty_name,
	tty_getty,
	tty_type,
	0, 
	tty_window,
	tty_comment
};

struct ttyent *getttyent()
{
	char *string;
	char line_of_file[1024];
	int current_pos;
 	
	if (ttyfile == NULL)
		if ((ttyfile = fopen("/etc/ttytab", "r")) == NULL)
			return NULL;

nextone:
	current_pos = 0;
	
	tty_name[0] = '\0';
	tty_getty[0] = '\0';
	tty_type[0] = '\0';
	tty_window[0] = '\0';
	tty_comment[0] = '\0';
	entry.ty_status = 0;

	if ((string = fgets(line_of_file, 1023, ttyfile)) == NULL)
		return NULL;
#ifdef DEBUG
	printf("getttyent: fgets returned '%s'\n", string);
#endif
	current_pos = getatoken(line_of_file, current_pos, tty_name, 31);

	if (*tty_name == '#')
		goto nextone;

	current_pos = getatoken(line_of_file, current_pos, tty_getty, 255);
	current_pos = getatoken(line_of_file, current_pos, tty_type, 31);
	current_pos = getatoken(line_of_file, current_pos, hmm, 255);

	if (!strncmp(hmm, "on", 2))
		entry.ty_status = TTY_ON;

	while (current_pos < strlen(line_of_file))
	{
		current_pos = getatoken(line_of_file, current_pos, hmm, 255);

		switch (hmm[0])
		{
		case 's':
			entry.ty_status |= TTY_SECURE;
			break;

		case 'w':
			strcpy(tty_window, &hmm[8]);
			tty_window[(strlen(tty_window) - 1)] = '\0';
			break;

		case '#':
			strcpy(tty_comment, &line_of_file[current_pos]);
			tty_comment[strlen(tty_comment) - 1] = '\0';
			current_pos = strlen(line_of_file);
			break;
		}
	}

	return &entry;
}

getatoken(input_string, position, output_string, maxlength)
char *input_string;
int position;
char *output_string;
int maxlength;
{
	register int i;
	char tmp_string[2048];
	
	while ((input_string[position] == ' ') || (input_string[position] == '\t'))
	{
#ifdef DEBUG
		printf("Killing white-space at position %d\n", position);
#endif
		position++;
	}

	if (input_string[position] == '"')
	{
#ifdef DEBUG
		printf("getatoken: quoted string '");
#endif
		for (position++, i = 0; (input_string[position] != '"') && (input_string[position] != '\0'); i++, position++)
		{
			tmp_string[i] = input_string[position];
#ifdef DEBUG
			printf("%c", tmp_string[i]);
#endif
		}

		position++;

		tmp_string[i] = '\0';
	}
	else
	{
#ifdef DEBUG
		printf("getatoken: unquoted string '");
#endif
		for (i = 0; !isspace(input_string[position]) && (input_string[position] != '\0'); i++, position++)
		{
			tmp_string[i] = input_string[position];
#ifdef DEBUG
			printf("%c", tmp_string[i]);
#endif
		}

		position++;
		
		tmp_string[i] = '\0';
	}

#ifdef DEBUG
	printf("'\n");
#endif

	strncpy(output_string, tmp_string, maxlength);

	return position;
}

setttyent()
{
	if (ttyfile != NULL)
	{
		fseek(ttyfile, 0L, 0);
		fflush(ttyfile);
	}
}

endttyent()
{
	if (ttyfile != NULL)
	{
		fclose(ttyfile);
		ttyfile = (FILE *) NULL;
	}
}

struct ttyent *getttynam(name)
char *name;
{
	struct ttyent *entry;
	
	while((entry = getttyent()) != NULL)
		if (!strcmp(entry->ty_name, name))
		{
			endttyent();
			return entry;
		}

	endttyent();

	return NULL;
}
