/*-
 * Copyright (c) 1980, 1987, 1988, 1991 The Regents of the University
 * of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980, 1987, 1988, 1991 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)login.c	5.73 (Berkeley) 6/29/91";
#endif /* not lint */

/*
 * login [ name ]
 * login -h hostname	(for telnetd, etc.)
 * login -f name	(for pre-authenticated login: datakit, xterm, etc.)
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>

#include <utmp.h>
#include <signal.h>
#include <errno.h>
#include <ttyent.h>
#include <syslog.h>
#include <grp.h>
#include <pwd.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#ifndef MINT
#include <tzfile.h>
#endif
#ifndef BSD44
#include <sys/types.h>
#include <lastlog.h>
extern char *index(), *rindex();
char *strerror();
typedef void *sig_t;
#define LOG_AUTHPRIV	LOG_AUTH
#ifdef MINT
#define L_SET 0
#endif
#endif

#include "pathnames.h"

#define	TTYGRPNAME	"tty"		/* name of group to own ttys */

/*
 * This bounds the time given to login.  Not a define so it can
 * be patched on machines where it's too small.
 */
int	timeout = 300;
int	rootlogin = 0;
#ifdef KERBEROS
int	notickets = 1;
char	*instance;
char	*krbtkfile_env;
int	authok;
#endif

struct	passwd *pwd;
int	failures;
#ifdef BSD44
char	term[64], *envinit[1], *hostname, *username, *tty;
#else
static char	term[64], *envinit[100], *hostname, *username, *tty;
#endif

main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	extern char *optarg, **environ;
	struct timeval tp;
	struct group *gr;
	register int ch;
	register char *p;
	int ask, fflag, hflag, pflag, cnt, uid;
	int quietlog, rval;
	char *domain, *salt, *ttyn;
	char tbuf[MAXPATHLEN + 2], tname[sizeof(_PATH_TTY) + 10];
	char localhost[MAXHOSTNAMELEN];
	char *ctime(), *ttyname(), *stypeof(), *crypt(), *getpass();
	time_t time();
	off_t lseek();
	void timedout();

	(void)signal(SIGALRM, timedout);
	(void)alarm((u_int)timeout);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
#ifndef MINT
	(void)setpriority(PRIO_PROCESS, 0, 0);
#endif

	openlog("login", LOG_ODELAY, LOG_AUTH);

	/*
	 * -p is used by getty to tell login not to destroy the environment
 	 * -f is used to skip a second login authentication 
	 * -h is used by other servers to pass the name of the remote
	 *    host to login so that it may be placed in utmp and wtmp
	 */
	domain = NULL;
	if (gethostname(localhost, sizeof(localhost)) < 0)
		syslog(LOG_ERR, "couldn't get local hostname: %m");
	else
		domain = index(localhost, '.');

	fflag = hflag = pflag = 0;
	uid = getuid();
	while ((ch = getopt(argc, argv, "fh:p")) != EOF)
		switch (ch) {
		case 'f':
			fflag = 1;
			break;
		case 'h':
			if (uid) {
				(void)fprintf(stderr,
				    "login: -h option: %s\n", strerror(EPERM));
				exit(1);
			}
			hflag = 1;
			if (domain && (p = index(optarg, '.')) &&
			    strcasecmp(p, domain) == 0)
				*p = 0;
			hostname = optarg;
			break;
		case 'p':
			pflag = 1;
			break;
		case '?':
		default:
			if (!uid)
				syslog(LOG_ERR, "invalid flag %c", ch);
			(void)fprintf(stderr,
			    "usage: login [-fp] [-h hostname] [username]\n");
			exit(1);
		}
	argc -= optind;
	argv += optind;
	if (*argv) {
		username = *argv;
		ask = 0;
	} else
		ask = 1;

	for (cnt = getdtablesize(); cnt > 2; cnt--)
		close(cnt);

	ttyn = ttyname(0);
	if (ttyn == NULL || *ttyn == '\0') {
		(void)sprintf(tname, "%s??", _PATH_TTY);
		ttyn = tname;
	}
	if (tty = rindex(ttyn, '/'))
		++tty;
	else
		tty = ttyn;

	for (cnt = 0;; ask = 1) {
		if (ask) {
			fflag = 0;
			getloginname();
		}
#ifdef	KERBEROS
		if ((instance = index(username, '.')) != NULL) {
			if (strncmp(instance, ".root", 5) == 0)
				rootlogin++;
			*instance++ = '\0';
		} else {
			rootlogin = 0;
			instance = "";
		}
#endif
		if (strlen(username) > UT_NAMESIZE)
			username[UT_NAMESIZE] = '\0';

		/*
		 * Note if trying multiple user names; log failures for
		 * previous user name, but don't bother logging one failure
		 * for nonexistent name (mistyped username).
		 */
		if (failures && strcmp(tbuf, username)) {
			if (failures > (pwd ? 0 : 1))
				badlogin(tbuf);
			failures = 0;
		}
		(void)strcpy(tbuf, username);

		if (pwd = getpwnam(username))
			salt = pwd->pw_passwd;
		else
			salt = "xx";

		/*
		 * if we have a valid account name, and it doesn't have a
		 * password, or the -f option was specified and the caller
		 * is root or the caller isn't changing their uid, don't
		 * authenticate.
		 */
		if (pwd && (*pwd->pw_passwd == '\0' ||
		    fflag && (uid == 0 || uid == pwd->pw_uid)))
			break;
		fflag = 0;
		if (pwd && pwd->pw_uid == 0)
			rootlogin = 1;

#ifndef MINT
		(void)setpriority(PRIO_PROCESS, 0, -4);
#endif

		p = getpass("Password:");
#ifdef MINTY
		printf("\n");
#endif

		if (pwd) {
#ifdef KERBEROS
			rval = klogin(pwd, instance, localhost, p);
			if (rval == 0)
				authok = 1;
			else if (rval == 1) {
				if (pwd->pw_uid != 0)
					rootlogin = 0;
				rval = strcmp(crypt(p, salt), pwd->pw_passwd);
			}
#else
			rval = strcmp(crypt(p, salt), pwd->pw_passwd);
#endif
		}
		bzero(p, strlen(p));

#ifndef MINT
		(void)setpriority(PRIO_PROCESS, 0, 0);
#endif

		/*
		 * If trying to log in as root without Kerberos,
		 * but with insecure terminal, refuse the login attempt.
		 */
#ifdef KERBEROS
		if (authok == 0)
#endif
		if (pwd && rootlogin && !rootterm(tty)) {
			(void)fprintf(stderr,
			    "%s login refused on this terminal.\n",
			    pwd->pw_name);
			if (hostname)
				syslog(LOG_NOTICE,
				    "LOGIN %s REFUSED FROM %s ON TTY %s",
				    pwd->pw_name, hostname, tty);
			else
				syslog(LOG_NOTICE,
				    "LOGIN %s REFUSED ON TTY %s",
				     pwd->pw_name, tty);
			continue;
		}

		if (pwd && !rval)
			break;

/* This next line added by S.R.Usher 24/5/94 to fix security bug */
		rootlogin = 0;
		
		(void)printf("Login incorrect\n");
		failures++;
		/* we allow 10 tries, but after 3 we start backing off */
		if (++cnt > 3) {
			if (cnt >= 10) {
				badlogin(username);
				sleepexit(1);
			}
			sleep((u_int)((cnt - 3) * 5));
		}
	}

	/* committed to login -- turn off timeout */
	(void)alarm((u_int)0);

	endpwent();

	/* if user not super-user, check for disabled logins */
	if (!rootlogin)
		checknologin();

	if (chdir(pwd->pw_dir) < 0) {
		(void)printf("No home directory %s!\n", pwd->pw_dir);
		if (chdir("/"))
			exit(0);
		pwd->pw_dir = "/";
		(void)printf("Logging in with home = \"/\".\n");
	}

	quietlog = access(_PATH_HUSHLOGIN, F_OK) == 0;

#ifdef BSD44
	if (pwd->pw_change || pwd->pw_expire)
		(void)gettimeofday(&tp, (struct timezone *)NULL);
	if (pwd->pw_change)
		if (tp.tv_sec >= pwd->pw_change) {
			(void)printf("Sorry -- your password has expired.\n");
			sleepexit(1);
		} else if (pwd->pw_change - tp.tv_sec <
		    2 * DAYSPERWEEK * SECSPERDAY && !quietlog)
			(void)printf("Warning: your password expires on %s",
			    ctime(&pwd->pw_expire));
	if (pwd->pw_expire)
		if (tp.tv_sec >= pwd->pw_expire) {
			(void)printf("Sorry -- your account has expired.\n");
			sleepexit(1);
		} else if (pwd->pw_expire - tp.tv_sec <
		    2 * DAYSPERWEEK * SECSPERDAY && !quietlog)
			(void)printf("Warning: your account expires on %s",
			    ctime(&pwd->pw_expire));
#endif /* BSD44 */
	/* nothing else left to fail -- really log in */
	{
		struct utmp utmp;

		bzero((void *)&utmp, sizeof(utmp));
		(void)time(&utmp.ut_time);
		strncpy(utmp.ut_name, username, sizeof(utmp.ut_name));
		if (hostname)
			strncpy(utmp.ut_host, hostname, sizeof(utmp.ut_host));
		strncpy(utmp.ut_line, tty, sizeof(utmp.ut_line));
		login(&utmp);
	}

	dolastlog(quietlog);

	chmod(ttyn, 0620);
	(void)chown(ttyn, pwd->pw_uid,
	    (gr = getgrnam(TTYGRPNAME)) ? gr->gr_gid : pwd->pw_gid);
	(void)setgid(pwd->pw_gid);

	initgroups(username, pwd->pw_gid);

	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = _PATH_BSHELL;

	/* destroy environment unless user has requested preservation */
	if (!pflag)
#ifdef BSD44
		environ = envinit;
#else
	{
		register int i;
		for (i = 0; i < 100 ; i++)
			envinit[i] = NULL;
	}
	else
	{
		register int i;
		register char *ptr;
		extern void *malloc();

		for (i = 0; ((ptr = environ[i]) != NULL); i++)
		{
			if ((envinit[i] = malloc(strlen(ptr))) == NULL)
				perror("copy_env");

			strcpy(envinit[i], ptr);
		}
	}

	{
		extern char *getenv();
		char *ptr = getenv("TERM");

		if (ptr != NULL)
			strcpy(term, ptr);
	}
#endif
	(void)setenv("HOME", pwd->pw_dir, 1);
	(void)setenv("SHELL", pwd->pw_shell, 1);
	if (term[0] == '\0')
		strncpy(term, stypeof(tty), sizeof(term));
	(void)setenv("TERM", term, 0);
	(void)setenv("LOGNAME", pwd->pw_name, 1);
	(void)setenv("USER", pwd->pw_name, 1);
	(void)setenv("PATH", _PATH_DEFPATH, 0);
#ifdef KERBEROS
	if (krbtkfile_env)
		(void)setenv("KRBTKFILE", krbtkfile_env, 1);
#endif

	if (tty[sizeof("tty")-1] == 'd')
		syslog(LOG_INFO, "DIALUP %s, %s", tty, pwd->pw_name);
	/* if fflag is on, assume caller/authenticator has logged root login */
	if (rootlogin && fflag == 0)
		if (hostname)
			syslog(LOG_NOTICE, "ROOT LOGIN (%s) ON %s FROM %s",
			    username, tty, hostname);
		else
			syslog(LOG_NOTICE, "ROOT LOGIN (%s) ON %s", username, tty);

#ifdef KERBEROS
	if (!quietlog && notickets == 1)
		(void)printf("Warning: no Kerberos tickets issued.\n");
#endif

	if (!quietlog) {
		struct stat st;
		printf(
"Copyright (c) 1980,1983,1986,1988,1990,1991 The Regents of the University\n%s",
"of California.  All rights reserved.\n\n");
		motd();
		(void)sprintf(tbuf, "%s/%s", _PATH_MAILDIR, pwd->pw_name);
		if (stat(tbuf, &st) == 0 && st.st_size != 0)
			(void)printf("You have %smail.\n",
			    (st.st_mtime > st.st_atime) ? "new " : "");
	}

	(void)signal(SIGALRM, SIG_DFL);
	(void)signal(SIGQUIT, SIG_DFL);
	(void)signal(SIGINT, SIG_DFL);
	(void)signal(SIGTSTP, SIG_IGN);

	tbuf[0] = '-';
	strcpy(tbuf + 1, (p = rindex(pwd->pw_shell, '/')) ?
	    p + 1 : pwd->pw_shell);

	if (setlogin(pwd->pw_name) < 0)
		syslog(LOG_ERR, "setlogin() failure: %m");

	/* discard permissions last so can't get killed and drop core */
	if (rootlogin)
		(void) setuid(0);
	else
		(void) setuid(pwd->pw_uid);
#ifndef MINT
	execlp(pwd->pw_shell, tbuf, 0);
#else
	execle(pwd->pw_shell, tbuf, 0, envinit);
#endif
	(void)fprintf(stderr, "%s: %s\n", pwd->pw_shell, strerror(errno));
	exit(1);
}

#ifdef	KERBEROS
#define	NBUFSIZ		(UT_NAMESIZE + 1 + 5) /* .root suffix */
#else
#define	NBUFSIZ		(UT_NAMESIZE + 1)
#endif

getloginname()
{
	register int ch;
	register char *p;
	static char nbuf[NBUFSIZ];

	for (;;) {
		(void)printf("login: ");
		for (p = nbuf; (ch = getchar()) != '\n'; ) {
			if (ch == EOF) {
				badlogin(username);
				exit(0);
			}
			if (p < nbuf + (NBUFSIZ - 1))
				*p++ = ch;
		}
		if (p > nbuf)
			if (nbuf[0] == '-')
				(void)fprintf(stderr,
				    "login names may not start with '-'.\n");
			else {
				*p = '\0';
				username = nbuf;
				break;
			}
	}
}

void
timedout()
{
	(void)fprintf(stderr, "Login timed out after %d seconds\n", timeout);
	exit(0);
}

rootterm(ttyn)
	char *ttyn;
{
	struct ttyent *t;

	return((t = getttynam(ttyn)) && t->ty_status&TTY_SECURE);
}

jmp_buf motdinterrupt;

motd()
{
	register int fd, nchars;
	sig_t oldint;
	void sigint();
	char tbuf[8192];

	if ((fd = open(_PATH_MOTDFILE, O_RDONLY, 0)) < 0)
		return;
	oldint = signal(SIGINT, sigint);
	if (setjmp(motdinterrupt) == 0)
		while ((nchars = read(fd, tbuf, sizeof(tbuf))) > 0)
			(void)write(fileno(stdout), tbuf, nchars);
	(void)signal(SIGINT, oldint);
	(void)close(fd);
}

void
sigint()
{
	longjmp(motdinterrupt, 1);
}

checknologin()
{
	register int fd, nchars;
	char tbuf[8192];

	if ((fd = open(_PATH_NOLOGIN, O_RDONLY, 0)) >= 0) {
		while ((nchars = read(fd, tbuf, sizeof(tbuf))) > 0)
			(void)write(fileno(stdout), tbuf, nchars);
		sleepexit(0);
	}
}

dolastlog(quiet)
	int quiet;
{
	struct lastlog ll;
	int fd;
	char *ctime();

	if ((fd = open(_PATH_LASTLOG, O_RDWR, 0)) >= 0) {
		(void)lseek(fd, (off_t)pwd->pw_uid * sizeof(ll), L_SET);
		if (!quiet) {
			if (read(fd, (char *)&ll, sizeof(ll)) == sizeof(ll) &&
			    ll.ll_time != 0) {
				(void)printf("Last login: %.*s ",
				    24-5, (char *)ctime(&ll.ll_time));
				if (*ll.ll_host != '\0')
					(void)printf("from %.*s\n",
					    sizeof(ll.ll_host), ll.ll_host);
				else
					(void)printf("on %.*s\n",
					    sizeof(ll.ll_line), ll.ll_line);
			}
			(void)lseek(fd, (off_t)pwd->pw_uid * sizeof(ll), L_SET);
		}
		bzero((void *)&ll, sizeof(ll));
		(void)time(&ll.ll_time);
		strncpy(ll.ll_line, tty, sizeof(ll.ll_line));
		if (hostname)
			strncpy(ll.ll_host, hostname, sizeof(ll.ll_host));
		(void)write(fd, (char *)&ll, sizeof(ll));
		(void)close(fd);
	}
}

badlogin(name)
	char *name;
{
	if (failures == 0)
		return;
	if (hostname) {
		syslog(LOG_NOTICE, "%d LOGIN FAILURE%s FROM %s",
		    failures, failures > 1 ? "S" : "", hostname);
		syslog(LOG_AUTHPRIV|LOG_NOTICE,
		    "%d LOGIN FAILURE%s FROM %s, %s",
		    failures, failures > 1 ? "S" : "", hostname, name);
	} else {
		syslog(LOG_NOTICE, "%d LOGIN FAILURE%s ON %s",
		    failures, failures > 1 ? "S" : "", tty);
		syslog(LOG_AUTHPRIV|LOG_NOTICE,
		    "%d LOGIN FAILURE%s ON %s, %s",
		    failures, failures > 1 ? "S" : "", tty, name);
	}
}

#undef	UNKNOWN
#define	UNKNOWN	"su"

char *
stypeof(ttyid)
	char *ttyid;
{
	struct ttyent *t;

	return(ttyid && (t = getttynam(ttyid)) ? t->ty_type : UNKNOWN);
}

sleepexit(eval)
	int eval;
{
	sleep((u_int)5);
	exit(eval);
}

#ifndef BSD44
setlogin()
{
}

setenv(what, value, idunno)
char *what;
char *value;
int idunno;
{
	static char buffer[32768];

	sprintf(buffer, "%s=%s", what, value);

	if (my_putenv(buffer, envinit))
		perror("my_putenv");
}

/* S.R.Usher 21/3/94
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
	extern void *malloc(), *realloc();
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

#include <errno.h>

char *sys_errlist[];

#ifndef MINT
char *strerror(error)
int error;
{
	return sys_errlist[error];
}
#endif

#define UTMP_FILE	"/etc/utmp"
#define WTMP_FILE	"/var/adm/wtmp"

login(utmp_ptr)
struct utmp *utmp_ptr;
{
	register int returned_val;
	int counter;
	int fd;
	struct utmp entry;

	char *line = utmp_ptr->ut_line;

	if ((fd = open(UTMP_FILE, O_RDWR)) == -1)
	{
		perror("write_utmp");
		return;
	}

	for (counter = 0; ((returned_val = read(fd, &entry, sizeof(struct utmp))) != -1); counter++)
	{
#ifdef DEBUG
		printf("Current line is '%s' (returned_val = %d)\n", entry.ut_line, returned_val);
#endif
		if (returned_val == 0)
			break;
		if (strncmp(line, entry.ut_line, 8) == 0)
			break;
	}

	if (lseek(fd, (counter * sizeof(struct utmp)), 0) == -1)
	{
		perror("write_utmp: lseek");
		close(fd);
		return;
	}

	if ((returned_val = write(fd, utmp_ptr, sizeof(struct utmp))) == -1)
		perror("write_utmp: write");
	else
		if (returned_val != sizeof(struct utmp))
			fprintf(stderr, "write_utmp: write: wrote too few bytes!\n");
#ifdef DEBUG
	printf("write_utmp: wrote %d bytes\n", returned_val);
#endif
	close(fd);

	counter = 0;

	if ((fd = open(WTMP_FILE, (O_RDWR | O_APPEND))) == -1)
	{
		printf("write_wtmp: %s\n", sys_errlist[errno]);
		return -1;
	}

	if (write(fd, utmp_ptr, sizeof(struct utmp)) == -1)
	{
		printf("write_wtmp: %s\n", sys_errlist[errno]);
		return -1;
	}

	close(fd);
}
#endif
