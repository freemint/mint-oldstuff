/*
 * Passwd.c for MiNT version 1.0 (c) S.R.Usher 1991-93.
 */

#define TMP_FILE "/etc/pwtmp"

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <pwd.h>

extern char *getenv();

struct passwd *pswdent, dummy;
struct sgttyb orig, noecho;

char *crypt();

static char *default_host = "this machine";

#ifndef MAX_NAME_LENGTH
#define MAX_NAME_LENGTH 80
#endif
#ifndef MAX_PASSWORD_LENGTH
#define MAX_PASSWORD_LENGTH 80
#endif

main(argc, argv)
int argc;
char *argv[];
{
	int i, nopasswd = 0, oktologin = 0, noentry = 0, starting_uid;
	int preserve, name_in_argv;
	char name[MAX_NAME_LENGTH], oldpassword[MAX_PASSWORD_LENGTH],
 		newpassword1[MAX_PASSWORD_LENGTH], newpassword2[MAX_PASSWORD_LENGTH];
	char objecttype[5], object[20], hostname[80];
	char key[3];

#if defined(atarist) && defined (PASSWD_FILE)
	setpwfile(PASSWD_FILE);
#else
#ifndef PASSWD_FILE
#define PASSWD_FILE "/etc/passwd"
#endif
#endif

	ioctl(fileno(stdin), TIOCGETP, &orig);
	ioctl(fileno(stdin), TIOCGETP, &noecho);

	noecho.sg_flags &= ~ECHO;

	if (parse_args(argc, argv, name, &preserve) == 0)
	{
		fail(name);
	}

	starting_uid = getuid();

	seteuid(0);
	setegid(0);

	pswdent = getpwnam(name);
	if (pswdent == NULL)
	{
		pswdent = &dummy;
		pswdent->pw_passwd = "\0";
		oktologin = 0;
		noentry = 1;
		printf("%s: %s: unknown user.\n", argv[0], name);
		exit(1);
	}
	else
	{
		if (strlen(pswdent->pw_passwd) == 0)
		{
			nopasswd = 1;
			oktologin = 1;
#ifdef DEBUG
			printf("Ooer! No password! (%s)\n", pswdent->pw_passwd);
#endif
		}
		noentry = 0;
	}

	gethostname(hostname, 79);

	if (strcmp("local", hostname) == 0)
		strcpy(hostname, default_host);

	strcpy(object, "password");
	
	printf("Changing %s for %s on %s.\n", object, pswdent->pw_name, hostname);

	if ((nopasswd == 0) && (starting_uid != 0))
	{
		ioctl(fileno(stdin), TIOCSETP, &noecho);

		printf("Old password:");
		fflush(stdout);
		fgets(oldpassword, MAX_PASSWORD_LENGTH, stdin);
		oldpassword[strlen(oldpassword) - 1] = '\0';
		printf("\n");

		ioctl(fileno(stdin), TIOCSETP, &orig);

		strncpy(key, pswdent->pw_passwd, 2);
		key[2] = '\0';

		if ((check_passwd((char *)(crypt(oldpassword, key)), pswdent->pw_passwd) == 1) && (noentry == 0))
			oktologin = 1;
	}


	endpwent();

	if (!oktologin)
	{
		printf("Sorry.\n");
		exit(1);
	}

	if ((oktologin == 1) && (noentry == 0))
	{
		ioctl(fileno(stdin), TIOCSETP, &noecho);

retry:		printf("New password:");
		fflush(stdout);
		fgets(newpassword1, MAX_PASSWORD_LENGTH, stdin);
		newpassword1[strlen(newpassword1) - 1] = '\0';
		printf("\n");

		if (strlen(newpassword1) < 6)
		{
			ioctl(fileno(stdin), TIOCSETP, &orig);
			printf("Please use a longer password.\n");
			goto retry;
		}

		printf("Retype new password:");
		fflush(stdout);
		fgets(newpassword2, MAX_PASSWORD_LENGTH, stdin);
		newpassword2[strlen(newpassword2) - 1] = '\0';
		printf("\n");

		ioctl(fileno(stdin), TIOCSETP, &orig);

		if (strcmp(newpassword1, newpassword2) == 0)
		{
			static char newbuf[16];

			generate_key(key);

			strcpy(newbuf, (char *)(crypt(newpassword1, key)));
			pswdent->pw_passwd = newbuf;
			setpwnam(name, pswdent);
		}
		else
		{
			printf("Mismatch - password unchanged.\n");
			exit(1);
		}
	}
	else
		fail(name);
}

check_passwd(given_pw, encrypted_pw)
char *given_pw;
char *encrypted_pw;
{
	if ((strncmp(given_pw, encrypted_pw, strlen(given_pw)) == 0) && (strlen(given_pw) == strlen(encrypted_pw)))
		return 1;
	else
		return 0;
}

generate_key(key)
char key[3];
{
	int value, i;

	srand(time(0L));

	for (i = 0; i < 2; i++)
	{
		value = rand();
		while ((key[i] = (char)((value % 94) + 33)) == ':')
			value = rand();
	}

	key[2] = '\0';
}

parse_args(argc, argv, name, preserve)
int argc;
char *argv[];
char *name;
{
	register int i = 1, j, retval;

	if (argc < 2)
	{
		strcpy(name, getenv("USER"));
		return 1;
	}

	if (argc > 3)
		usage(argv);

	if (i < argc)
	{
		if ((j = strlen(argv[i])) > 8)
			j = 8;
		strncpy(name, argv[i], j);
		name[8] = '\0';
	}

	if (getuid())
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

usage(argv)
char *argv[];
{
	fprintf(stderr, "Usage: %s [-p] [name]\n", argv[0]);
	exit(0);
}

fail(name)
char *name;
{
	char hostname[80];

	gethostname(hostname, 79);
	fprintf(stderr, "Can't change password for %s on %s\n", name, hostname);
	exit(1);
}

setpwnam(name, entry)
char *name;
struct passwd *entry;
{
	FILE *fp1, *fp2;
	char record[256], testnam[80];

	if ((fp1 = fopen(PASSWD_FILE, "r")) == NULL)
		fail(name);

	if ((fp2 = fopen(TMP_FILE, "w")) == NULL)
	{
		fclose(fp1);
		fail(name);
	}

	while(fgets(record, 1024, fp1) != NULL)
	{
		sscanf(record, "%s:", testnam);

		if (strncmp(testnam, name, strlen(name)) == 0)
			create_entry(fp2, entry);
		else
			fwrite(record, strlen(record), 1, fp2);
	}

	fclose(fp1);
	fclose(fp2);

	if (unlink(PASSWD_FILE))
	{
#ifdef DEBUG
		perror(PASSWD_FILE);
#endif
		fail(name);
	}
	
	if (rename(TMP_FILE, PASSWD_FILE))
	{
#ifdef DEBUG
		perror(TMP_FILE);
#endif
		fail(name);
	}
}

create_entry(file, entry)
FILE *file;
struct passwd *entry;
{
	fprintf(file, "%s:%s:%d:%d:%s:%s:%s\n",
		entry->pw_name,
		entry->pw_passwd,
		entry->pw_uid,
		entry->pw_gid,
		entry->pw_gecos,
		entry->pw_dir,
		entry->pw_shell);

	fflush(file);
}
