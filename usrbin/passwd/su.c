/*
 * Su.c version 1.3 (c) S.R.Usher 1993.
 *	Based upon login.c version 1.1 (c) S.R.Usher 1991/92/93.
 *
 * Changelog
 * ---------
 *
 *  2/8/93	S.R.Usher	1.1	Added system logger message code.
 *
 *  28/8/93	S.R.Usher	1.2	Fixed parse_args, username wasn't
 *						being terminated properly.
 *
 * 24/11/93	S.R.Usher	1.3	Plugged possible security hole, ie.
 *						stopped using gets() and used
 *						fgets(). The password given
 *						can nolonger overflow its
 *						buffer!
 *
 */

#include <stdio.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <pwd.h>
#include <lastlog.h>
#include <ttyent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>

/* default shell */
#define SHELL "/bin/sh"

struct passwd *pswdent, dummy;
struct sgttyb orig, noecho;

void sighandler();
char *crypt();
void *malloc(), *realloc(), *strchr();

time_t last_time();

extern char **environ;

int env_index;

struct ttyent *ttyentry;
char ourtty[32];
int secure;

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
	int i, nopasswd = 0, oktologin = 0, noentry = 0;
	int preserve, name_in_argv;
	char name[MAX_NAME_LENGTH], password[MAX_PASSWORD_LENGTH];
	char ourname[MAX_NAME_LENGTH];
	int ouruid;
	char key[3];
	char *ptr;

	signal(SIGALRM, sighandler);
	sigblock(sigmask(SIGTTOU));
	sigblock(sigmask(SIGTTIN));
	sigblock(sigmask(SIGSTOP));

	if ((ouruid =getuid()) == 0)
		nopasswd = oktologin = 1;

	seteuid(0);
	setegid(0);

	if ((pswdent = getpwuid(ouruid)) != NULL)
		strncpy(ourname, pswdent->pw_name, 79);
	else
		sprintf(ourname, "uid %d", ouruid);

	strcpy(ourtty, (ptr = (char *) ttyname(0)));

	if ((ptr = (char *) strrchr(ptr, '/')) != NULL)
		strcpy(ourtty, ++ptr);

	ioctl(fileno(stdin), TIOCGETP, &orig);
	ioctl(fileno(stdin), TIOCGETP, &noecho);

	noecho.sg_flags &= ~ECHO;

	preserve = 1;

	name_in_argv = parse_args(argc, argv, name, &preserve);

	if (name_in_argv == 0)
		strcpy(name, "root");

	pswdent = getpwnam(name);

	if (pswdent == NULL)
	{
		pswdent = &dummy;
		pswdent->pw_passwd = "\0";
		oktologin = 0;
		noentry = 1;
#ifdef DEBUG
		printf("Ooer! No entry!\n");
#endif
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
		
	if (nopasswd == 0)
	{
		ioctl(fileno(stdin), TIOCSETP, &noecho);
#ifdef OWN_TIMEOUT
		alarm(60);
#endif
		printf("Password:");
		fflush(stdout);
		fgets(password, MAX_PASSWORD_LENGTH, stdin);
		password[strlen(password) - 1] = '\0';
		printf("\n");

		ioctl(fileno(stdin), TIOCSETP, &orig);

		strncpy(key, pswdent->pw_passwd, 2);
		key[2] = '\0';

		if ((check_passwd((char *)(crypt(password, key)), pswdent->pw_passwd) == 1) && (noentry == 0))
			oktologin = 1;
	}

#ifdef DEBUG
	printf("oktologin = %s noentry = %s secure = %s\n", (oktologin == 1 ? "TRUE" : "FALSE"), (noentry == 1 ? "TRUE" : "FALSE"), (secure == 1 ? "TRUE" : "FALSE"));
#endif
	if ((oktologin == 1) && (noentry == 0))
	{
		alarm(0);

		inform_of_success(name, ourname);

		execit(preserve);
	}

	printf("Sorry\n");

	inform_of_failure(name, ourname);
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

execit(preserve)
int preserve;
{
	static char name[80];
	static char argvzero[80];
	char *pointer, tmpstring[80], *tmpptr, *tmpptr2;
	time_t my_lasttime;
	static char *protoenv[100];
	char *shellname = SHELL;
	char extendshellnam[256];

	if (pswdent->pw_shell != NULL && *pswdent->pw_shell != '\0')
		shellname = pswdent->pw_shell;
	else
		shellname = SHELL;

	if ((pointer = (char *) strrchr(shellname, '/')) == NULL)
		pointer = shellname;
	else
		pointer++;
	strcpy(tmpstring, pointer);

	if ((pointer = (char *) strrchr(tmpstring, '.')) != NULL)
		*pointer = '\0';

	if (!preserve)
		sprintf(argvzero, "-su");
	else
		sprintf(argvzero, "su");

#ifdef DEBUG
	printf("argvzero = '%s'\n", argvzero);
#endif

	if (preserve == 1)
		env_index = copy_environment(protoenv);
	else
		env_index = 0;


	sprintf(tmpstring, "HOME=%s", pswdent->pw_dir);
	my_putenv(tmpstring, protoenv);
	
	if (!preserve)
	{
		if (chdir(pswdent->pw_dir) == -1)
		{
			sprintf(tmpstring, "HOME=/");
			my_putenv(tmpstring, protoenv);
			printf("\nNo home directory. home=/\n");
			chdir("/");
		}
	}

	if (ourtty == NULL)
	{
		my_putenv("TTY=console", protoenv);
	}
	else
	{
		sprintf(tmpstring, "TTY=%s", ourtty);
#ifdef DEBUG
		printf("%s\n", tmpstring);
#endif
		my_putenv(tmpstring, protoenv);
	}

	if (setuid(pswdent->pw_uid) == -1)
	{
#ifndef MINT
		fprintf(stderr, "login: can't set userid (errno = %d)\n", errno);
		exit(0);
#endif
	}
	if (setgid(pswdent->pw_gid) == -1)
	{
#ifndef MINT
		fprintf(stderr, "login: can't set groupid (errno = %d)\n", errno);
		exit(0);
#endif
	}
	
	sprintf(tmpstring, "USER=%s", pswdent->pw_name);
#ifdef DEBUG
	printf("putenv(\"%s\") returned %d\n", tmpstring, 
#endif
	my_putenv(tmpstring, protoenv)
#ifdef DEBUG
	)
#endif
	;

	execle(shellname, argvzero, NULL, protoenv);

	sprintf(extendshellnam, "%s.ttp", shellname);
	execle(extendshellnam, argvzero, NULL, protoenv);

	sprintf(extendshellnam, "%s.prg", shellname);
	execle(extendshellnam, argvzero, NULL, protoenv);

	sprintf(extendshellnam, "%s.gtp", shellname);
	execle(extendshellnam, argvzero, NULL, protoenv);


	printf("\nNo Shell\n");
	exit();
}

void sighandler()
{
	printf("\nTimed out after 60 seconds.\n");
	ioctl(fileno(stdin), TIOCSETP, &orig);
	exit();
}

parse_args(argc, argv, name, preserve)
int argc;
char *argv[];
char *name;
int *preserve;
{
	register int i = 1, j;

	if (argc < 2)
	{
		*preserve = 1;
		name[0] = '\0';
		return 0;
	}

	if (argc > 3)
		usage(argv);

	if (argv[i][0] == '-')
		if (argv[i++][1] == '\0')
			*preserve = 0;
		else
			usage(argv);

	if (i < argc)
	{
		if ((j = strlen(argv[i])) > 8)
			j = 8;
		strncpy(name, argv[i], j);
		name[j] = '\0';
		return 1;
	}
	else
	{
		name[0] = '\0';
		return 0;
	}
}

usage(argv)
char *argv[];
{
	fprintf(stderr, "Usage: %s [-] [name]\n", argv[0]);
	exit(0);
}

check_nologin()
{
	FILE *fp;
	char string[1024];

	if ((fp = fopen("/etc/nologin", "r")) == NULL)
	{
		return;
	}

	while(fgets(string, 1024, fp) != NULL)
		printf("%s", string);

	exit(0);
}

copy_environment(protoenv)
char *protoenv[];
{
	register int i;
	register char *ptr;

	for (i = 0; ((ptr = environ[i]) != NULL); i++)
	{
		if ((protoenv[i] = malloc(strlen(ptr) + 1)) == NULL)
			exit(1);

		strcpy(protoenv[i], ptr);
	}

	protoenv[i] = NULL;

	return i;
}

/*
 * This is my putenv routine, it first searches for an environment variable of
 * the same name as the one to be added to the environment. If does find one,
 * it attempts to reallocate the memory for the original string to hold the
 * new one. If it doesn't find it then it malloc()s enough memory for the
 * string and adds the pointer to the envronment pointer array.
 *
 * It was written so as to add entries to an environment list which may not
 * be the standard one.
 */

my_putenv(string, environment)
char *string;
char *environment[];
{
	register int i;
	char match_string[80];
	char *val_ptr, *env_ptr;
	int string_len;

	if ((string_len = strlen(string)) == 0)
		return -1;
	
	string_len++;

	if ((val_ptr = strchr(string, '=')) == NULL)
		return -1;

	*val_ptr++ = '\0';

	strcpy(match_string, string);

	for (i = 0; ((env_ptr = environment[i]) != NULL); i++)
	{
		if (!strncmp(env_ptr, match_string, strlen(match_string)))
		{
			if ((env_ptr = realloc(env_ptr, string_len)) == NULL)
				return -1;
			sprintf(env_ptr, "%s=%s", match_string, val_ptr);
			environment[i] = env_ptr;
			return 0;
		}
	}

	if ((env_ptr = malloc(string_len)) == NULL)
		return -1;

	sprintf(env_ptr, "%s=%s", match_string, val_ptr);

	environment[i] = env_ptr;

	return 0;
}

#include <sys/syslog.h>

inform_of_success(towho, username)
char *towho;
char *username;
{
	openlog("su", 0, LOG_AUTH);
	syslog(LOG_NOTICE, "'su %s' succeeded for %s on %s", towho, username, ourtty);
	closelog();
}

inform_of_failure(towho, username)
char *towho;
char *username;
{
	openlog("su", 0, LOG_AUTH);
	syslog(LOG_NOTICE, "'su %s' failed for %s on %s", towho, username, ourtty);
	closelog();
}
