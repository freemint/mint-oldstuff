/*
 * Login.c for MiNT version 1.4 (c) S.R.Usher 1991/92/93.
 *
 * Changelog
 * ---------
 *
 * 25/9/91	S.R.Usher	0.2	Added encrypted passwords using
 *					 Alec "Igor" Muffett's fcrypt()
 *					 routine from his password cracker.
 *					 (Thank's Alec!)
 *
 * 20/10/91	S.R.Usher	0.3	Added support for BSD style utmp/wtmp.
 *
 * 10/11/91	S.R.Usher	0.4	Added Dave Gymer's patches & started
 *					using my my_putenv() rather than
 *					Rob Newson's my_putenv().
 *
 * 18/1/92	S.R.Usher	0.5	Changed from using getenv("TTY") to
 *					using ttyname(0). Added seteuid() and
 *					setegid() so that this should work
 *					when used on filesystems with
 *					protection.
 *
 * 22/1/92	S.R.Usher	0.6	Moved the no home directory message
 *					before the motd etc, where it should
 *					have been in the first place!
 *
 * 13/9/92	S.R.Usher	0.7	Added support for random key held in
 *				(1.0)	passwd file as the first two
 *					characters of the encrypted password.
 *
 *  8/3/93	S.R.Usher	1.1	Reimplement changes made on version
 *					I accidently deleted.. oh well.
 *					Firstly, for some unknown reason this
 *					program doesn't run properly in
 *					TT-RAM, so don't try it.
 *					UID and GID are now set after utmp
 *					and wtmp are updated.
 *					Mail notification message depends
 *					upon whether mail arrived since last
 *					login.
 *
 *  2/8/93	S.R.Usher	1.2	Added system logger message logging
 *					code.
 *
 *  28/8/93	S.R.Usher	1.3	Fixed parse_args, username wasn't
 *						being terminated properly.
 *
 * 24/11/93	S.R.Usher	1.4	We don't use gets() any more, twas a
 *						security hole! Also, made
 *						sure that the login name and
 *						password given by the user
 *						don't overflow.
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
#ifdef MINT
# ifndef SHELL
#  define SHELL "/bin/sh.ttp" 
# endif
#else
#define SHELL "/bin/sh"
#endif

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
	char key[3];
	char *ptr;

	signal(SIGALRM, sighandler);
	sigblock(sigmask(SIGTTOU));
	sigblock(sigmask(SIGTTIN));
	sigblock(sigmask(SIGSTOP));

	seteuid(0);
	setegid(0);

	check_nologin();

	strcpy(ourtty, (ptr = (char *) ttyname(0)));

	if ((ptr = (char *) strrchr(ptr, '/')) != NULL)
		strcpy(ourtty, ++ptr);

	if ((ttyentry = getttynam(ourtty)) != NULL)
	{
		secure = (ttyentry->ty_status & TTY_SECURE);
	}
#ifdef DEBUG
	else
		fprintf(stderr, "getttynam() returned NULL for %s", ourtty);
#endif

	ioctl(fileno(stdin), TIOCGETP, &orig);
	ioctl(fileno(stdin), TIOCGETP, &noecho);

	noecho.sg_flags &= ~ECHO;

	name_in_argv = parse_args(argc, argv, name, &preserve);

	for (i = 0; i < 4; i++)
	{
		if (name_in_argv == 0)
		{
again:
			printf("login: ");
			if (getstring(name, MAX_NAME_LENGTH) == EOF)
				exit(0);
			if (strlen(name) == 0)
				goto again;
		}
		else
		{
			name_in_argv = 0;
		}

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

		if (pswdent->pw_name != NULL && !strcmp(pswdent->pw_name, "root") && !secure)
			oktologin = 0;

#ifdef DEBUG
		printf("oktologin = %s noentry = %s secure = %s\n", (oktologin == 1 ? "TRUE" : "FALSE"), (noentry == 1 ? "TRUE" : "FALSE"), (secure == 1 ? "TRUE" : "FALSE"));
#endif
		if ((oktologin == 1) && (noentry == 0))
		{
			alarm(0);

			execit(preserve);
		}
		else
			printf("Login incorrect\n");
	}
	printf("Too many tries.\n");

	inform_of_failure(name);
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

	sprintf(argvzero, "-%s", tmpstring);

#ifdef DEBUG
	printf("argvzero = '%s'\n", argvzero);
#endif

	if (preserve == 1)
		env_index = copy_environment(protoenv);
	else
		env_index = 0;


	if (chdir(pswdent->pw_dir) != -1)
	{
		sprintf(tmpstring, "HOME=%s", pswdent->pw_dir);
		my_putenv(tmpstring, protoenv);
		chdir(pswdent->pw_dir);
	}
	else
	{
		sprintf(tmpstring, "HOME=/");
		my_putenv(tmpstring, protoenv);
		printf("\nNo home directory. home=/\n");
		chdir("/");
	}

	sprintf(tmpstring, "%s/.hushlogin", pswdent->pw_dir);

	if (access(tmpstring, F_OK) != 0)
	{
		my_lasttime = last_time();
		check_motd();
		check_mail(my_lasttime);
		if (pswdent->pw_uid == 0)
			inform_of_root();
	}

	sprintf(tmpstring, "/dev/%s", ourtty);
	if (chown(tmpstring, pswdent->pw_uid, pswdent->pw_gid) == -1)
	{
#ifndef MINT
		fprintf(stderr, "login: can't set ownership for %s\n", tmpstring);
		exit(0);
#endif
	}


	if (ourtty == NULL)
	{
		write_wtmp("console", pswdent->pw_name, "\0", time(0L));
		write_utmp("console", pswdent->pw_name, "\0", time(0L));
		my_putenv("TTY=console", protoenv);
	}
	else
	{
		sprintf(tmpstring, "TTY=%s", ourtty);
#ifdef DEBUG
		printf("%s\n", tmpstring);
#endif
		my_putenv(tmpstring, protoenv);
		write_wtmp(ourtty, pswdent->pw_name, "\0", time(0L));
		write_utmp(ourtty, pswdent->pw_name, "\0", time(0L));
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
		*preserve = 0;
		name[0] = '\0';
		return 0;
	}

	if (argc > 3)
		usage(argv);

	if (argv[i][0] == '-')
		if (argv[i++][1] == 'p')
			*preserve = 1;
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
	fprintf(stderr, "Usage: %s [-p] [name]\n", argv[0]);
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

check_motd()
{
	FILE *fp;
	char string[1024];

	if ((fp = fopen("/etc/motd", "r")) == NULL)
	{
		return;
	}

	while(fgets(string, 1024, fp) != NULL)
		printf("%s", string);

	fclose(fp);
	
	return;
}

check_mail(lastlogin)
time_t lastlogin;
{
	char string[1024];
	struct stat info;

	sprintf(string, "/usr/spool/mail/%s", pswdent->pw_name);

	if (!stat(string, &info) && info.st_size > 0)
		printf("You have %smail.\n", ((((long)info.st_mtime - (long)lastlogin) > 0) ? "new " : "\0"));
}

time_t last_time()
{
	struct lastlog logent;
	FILE *fp;
	char *how, *my_line, output_string[80], *ptr;
	struct tm *some_time;
	time_t returnval;

	static char *dow[7] = {
		"Sun",
		"Mon",
		"Tue",
		"Wed",
		"Thu",
		"Fri",
		"Sat"
	};

	static char *moy[12] = {
		"Jan",
		"Feb",
		"Mar",
		"Apr",
		"May",
		"Jun",
		"Jul",
		"Aug",
		"Sep",
		"Oct",
		"Nov",
		"Dec"
	};

	if (access("/var/adm/lastlog", F_OK) == 0)
		how = "r+";
	else
		how = "w";

	if ((fp = fopen("/var/adm/lastlog", how)) == NULL)
	{
		perror("/var/adm/lastlog");
		return;
	}

	if (fseek(fp, (pswdent->pw_uid * sizeof(struct lastlog)), 0) != 0)
	{
		perror("/var/adm/lastlog");
		fclose(fp);
		return;
	}

	if ((fread(&logent, sizeof(struct lastlog), 1, fp) != 1) || (logent.ll_time == 0))
	{
		sprintf(output_string, "Never logged in.\n");
		returnval = (time_t) 0;
	}
	else
	{
		some_time = localtime(&logent.ll_time);
		sprintf(output_string,
			"Last login: %s %s %02d %02d:%02d:%02d on %s\n",
			dow[some_time->tm_wday],
			moy[some_time->tm_mon],
			some_time->tm_mday,
			some_time->tm_hour,
			some_time->tm_min,
			some_time->tm_sec,
			logent.ll_line);
		returnval = logent.ll_time;
	}

	printf(output_string);

	logent.ll_time = time(0);
	my_line = ourtty;
	if (my_line == NULL)
		my_line = "console";
			
	strcpy(logent.ll_line, my_line);
	logent.ll_host[0] = '\0';

	if (fseek(fp, (pswdent->pw_uid * sizeof(struct lastlog)), 0) != 0)
	{
		perror("/var/adm/lastlog");
		fflush(fp);
		fclose(fp);
		return;
	}

	if (fwrite(&logent, sizeof(struct lastlog), 1, fp) != 1)
		perror("Writing lastlog");

	fflush(fp);
	fclose(fp);

	return returnval;
}

getstring(string, length)
char *string;
int length;
{
	register int i, c;

	*string = '\0';

	for (i = 0; (c = fgetc(stdin)) != '\n' && (i < length); i++)
	{
		if ((c == '\004') || (c == EOF))
		{
			printf("^D\n");
			return EOF;
		}

		if (c < 32)
			printf("^%c", (c + 0x40));

		string[i] = c;
	}

	string[i] = '\0';

	return 0;
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

inform_of_root()
{
	int facility = 0;

	openlog("login", 0, LOG_AUTH);
	syslog(LOG_NOTICE, "ROOT LOGIN %s", ourtty);
	closelog();
}

#ifdef HOSTNAMEGIVEN
inform_of_failure(username, hostname)
char *username;
char *hostname;
#else
inform_of_failure(username)
char *username;
#endif
{
	int facility = 0;

	openlog("login", 0, LOG_AUTH);
#ifdef HOSTNAMEGIVEN
	if (hostname != NULL)
		syslog(LOG_NOTICE, "REPEATED LOGIN FAILURES ON %s FROM %s, %s", ourtty, hostname, username);
	else
#else
		syslog(LOG_NOTICE, "REPEATED LOGIN FAILURES ON %s, %s", ourtty, username);
#endif
	closelog();
}
