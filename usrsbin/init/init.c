/*
 * Init.c for MiNT version 1.5 (c) S.R.Usher 1991/92/93/94.
 *
 * Changelog
 *
 * 21/09/91	0.1	S.R.Usher	Original version.
 *
 * 20/10/91	0.2	S.R.Usher	Added support for BSD style
 * 					utmp/wtmp and made it able to be
 *					shutdown!
 *
 * 29/11/91	0.3	S.R.Usher	Make sure the ttytab file is closed
 *					after reading it!
 *
 * 27/12/91	0.4	S.R.Usher	Kill all processes except MiNT &
 *					init(8) when doing a shutdown or
 *					reboot as single user. Fix code for
 *					re-reading ttytab.
 *
 * 18/1/92	0.5	S.R.Usher	Following suggestions from Dave Gymer
 *		(1.0)			the Pdomain call was removed as was
 *					the call to putenv to set up the TTY
 *					variable. Also, the presents of the
 *					rc files is checked before sh is run
 *					with them.
 *
 * 8/3/93	1.1	S.R.Usher	Change the way version info is
 *					printed.
 *					Changed way the shell is run from
 *					using system() to using fork()+exec()
 *
 * 24/11/93	1.2	S.R.Usher	Changed time at which SIGINT etc are
 *					noticed.
 *
 * 9/4/94	1.3	S.R.Usher	Rescheduled time at which rc.boot
 *					and rc.single are run and the time
 *					at which wtmp and utmp are updated.
 *					Reset ownership of tty before getty
 *					is run.
 *					Added code to set up console for
 *					single user mode.
 *
 * 20/6/94	1.4	tesche		Check for ability to open the console
 *					and check if terminal exists before
 *					trying to run a getty.
 *
 * 3/8/94	1.5	S.R.Usher	Made sure we exit when told to
 *					shutdown. Changed startup message
 *					when compiled under MiNT.
 *
 * 27/8/94	1.6	S.R.Usher	Make sure all messages go to console
 *					rather than stdout or stderr.
 *
 */

#define VERSION 1.6

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <ttyent.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifdef MINT
#include <ioctl.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <mintbind.h>

#include "version.h"

extern char *strchr();
#endif

#define SKIP_RC_BOOT	0x01
#define SINGLE_USER	0x02
#define HALT		0x10

static int slowdown = 0;
static int shutdown = 0;
static int exit_anyway = 0;
static int times_up = 0;
static int updatepid = 0;

int flags;

void reread_ttys(), do_single_user(), cease_new_ttys(), generic(), do_shutdown();
void handle_sigalrm();

#ifdef BANANA
char *version = "\033pinit version %2.1f (c) S.R.Usher 1991/2/3/4 (Built %s %s)\033q\033e\033v\n";
#else
char *version = "\033pBSD-like init Release %2.1f (%s %s).\033q\033e\033v\nCopyright (c) 1991-1994, S.R.Usher.\n\n";
#endif
#ifdef MINT
char *mintosversion = MINTOSVERS;
#endif

static FILE *console;

main(argc, argv, envp)
int argc;
char *argv[];
char *envp[];
{
	register FILE *fp;
	register int i;

	signal(SIGTTOU, generic);
	signal(SIGTTIN, generic);

	sigblock(sigmask(SIGPIPE));
	sigblock(sigmask(SIGURG));
	sigblock(sigmask(SIGSTOP));
	sigblock(sigmask(SIGTTOU));
	sigblock(sigmask(SIGTTIN));
	sigblock(sigmask(SIGCONT));
	sigblock(sigmask(SIGCHLD));
	sigblock(sigmask(SIGIO));
	sigblock(sigmask(SIGXCPU));
	sigblock(sigmask(SIGXFSZ));
	sigblock(sigmask(SIGVTALRM));
	sigblock(sigmask(SIGPROF));
	sigblock(sigmask(SIGWINCH));
	sigblock(sigmask(SIGUSR1));
	sigblock(sigmask(SIGUSR2));

	if (!(console = fopen("/dev/console", "w+"))) {
		/* looks like we're in problems... :-( */
		exit(-1);
	}

#ifdef MINT
	fprintf(console, "%s\033e\033v\n", mintosversion);
#else
	fprintf(console, version, VERSION, __TIME__, __DATE__ );
#endif

	flags = parse_opts(argc, argv);

	signal(SIGHUP, reread_ttys);
	signal(SIGINT, do_single_user);
	signal(SIGTERM, do_shutdown);
	signal(SIGTSTP, cease_new_ttys);
	signal(SIGALRM, handle_sigalrm);

reboot:
	if (!(SKIP_RC_BOOT & flags) && !access("/etc/rc.boot", F_OK) &&
			!access("/bin/sh", F_OK))
		if (fork())
			wait(0L);
		else
			execl("/bin/sh", "/bin/sh", "/etc/rc.boot", 0L);
#ifdef DEBUG
	else
		fprintf(console, "Can't access /etc/rc.boot\n");
#endif
		
	if (SINGLE_USER & flags)
		single_user();

	if (write_wtmp("~", "reboot", "\0", time(0L)) == -1)
		fprintf(console, "init: reboot: writing wtmp entry: %s\n", sys_errlist[errno]);

/*
 * Clear out the utmp file...
 */
	unlink("/etc/utmp");
	
/*
 * Re-create it... this is cludge... I don't know why, but creat() produces a
 * file of zero length which acts rather like /dev/null, the only thing which
 * gets bigger when you write to it is the size given by ls(1)!
 */
	if ((fp = fopen("/etc/utmp", "w")) != NULL)
	{
		fclose(fp);
	}

	if (!access("/etc/rc", F_OK) && !access("/bin/sh", F_OK))
		if (fork())
			wait(0L);
		else
			execl("/bin/sh", "/bin/sh", "/etc/rc", 0L);
#ifdef DEBUG
	else
		fprintf(console, "Can't access /etc/rc\n");
#endif

	get_tty_setup();

	run_ttys();

	if ((flags & HALT) != HALT)
	{
		shutdown = 0;
		goto reboot;
	}

	if (write_wtmp("~", "shutdown", "\0", time(0L)) == -1)
		fprintf(console, "init: shutdown: writing wtmp entry: %s\n", sys_errlist[errno]);
	exit(0);
}

parse_opts(argc, argv)
int argc;
char *argv[];
{
	int flag = 0;
	int i, j;
	
	if (argc < 2)
		return flag;

	for (i = 0; i < argc; i++)
	{
		if (argv[i][0] == '-')
			for (j = 1; argv[i][j] != '\0'; j++)
				switch (argv[i][j])
				{
				case 'b':
					flag |= SKIP_RC_BOOT;
					break;

				case 's':
					flag |= SINGLE_USER;
					break;

				default:
					fprintf(stderr, "init: unknown option\n");
				}
	}

	return flag;
}

#include <sys/termios.h>

#define TTYDEF_IFLAG BRKINT | IGNPAR | ICRNL | IXON
#ifdef ONLCR
#define TTYDEF_OFLAG ONLCR
#else
#define TTYDEF_OFLAG OPOST
#endif
#define TTYDEF_LFLAG ISIG | ICANON | ECHO | ECHOE | ECHONL | IEXTEN
#define TTYDEF_CFLAG CS8 | CREAD | PARENB

single_user()
{
	struct termios term;

	if (!access("/etc/rc.single", F_OK) && !access("/bin/sh", F_OK))
		if (fork())
			wait(0L);
		else
			execl("/bin/sh", "/bin/sh", "/etc/rc.single", 0L);

	tcgetattr(fileno(console), &term);
	term.c_iflag = TTYDEF_IFLAG;
	term.c_oflag = TTYDEF_OFLAG;
	term.c_lflag = TTYDEF_LFLAG;
	term.c_cflag = TTYDEF_CFLAG;
	tcsetattr(fileno(console), TCSAFLUSH, &term);

	if (fork())
		wait(0L);
	else
		execl("/bin/sh", "/bin/sh", 0L);
}

struct my_tty {
	char mtt_name[32];
	int mtt_pid;
	int mtt_flag;
};

#define MAXENTS 32

static struct my_tty myttys[MAXENTS];
static entries[MAXENTS];
static num_of_ents = 0;

get_tty_setup()
{
	register int i;
	struct ttyent *entry;
	
	setttyent();

	num_of_ents = 0;

	for (i = 0; ((entry = getttyent()) != NULL) && (i < MAXENTS); i++, num_of_ents++)
	{
#ifdef DEBUG
		fprintf(console, "init: get_tty_setup: getttyent returned entry for %s.\n", entry->ty_name);
#endif
		entries[i] = 1;
		strcpy(myttys[i].mtt_name, entry->ty_name);
		myttys[i].mtt_pid = 0;
		myttys[i].mtt_flag = entry->ty_status;
	}
#ifdef DEBUG
	fprintf(console, "num_of_ents = %d\n", num_of_ents);
#endif
	endttyent();
}

void reread_ttys()
{
	register int i;
	struct ttyent *entry;
	
	setttyent();

	num_of_ents = 0;

	for (i = 0; (((entry = getttyent()) != NULL) && (i < MAXENTS)); i++, num_of_ents++)
	{
#ifdef DEBUG
		fprintf(console, "init: reread_ttys: getttyent returned entry for %s.\n", entry->ty_name);
#endif
		if ((myttys[i].mtt_flag & TTY_ON) && !(entry->ty_status & TTY_ON))
		{
#ifdef DEBUG
			fprintf(console, "init: reread_ttys: killing %s (pid %d)\n", myttys[i].mtt_name, myttys[i].mtt_pid);
#endif
			killpg(myttys[i].mtt_pid, SIGKILL);
			myttys[i].mtt_pid = 0;
		}

		if (((myttys[i].mtt_flag & TTY_ON) == 0) && ((entry->ty_status & TTY_ON) > 0))
		{
			strcpy(myttys[i].mtt_name, entry->ty_name);
#ifdef DEBUG
			fprintf(console, "init: reread_ttys: Starting new tty, %s... flag = %d (%d).\n", myttys[i].mtt_name, (entry->ty_status & TTY_ON), i);
#endif
			myttys[i].mtt_pid = start_getty_entry(entry, myttys[i].mtt_name);
		}
		
		myttys[i].mtt_flag = entry->ty_status;
	}
	endttyent();
}

run_ttys()
{
	register int i;
	int pid;

	for (i = 0; i < MAXENTS; i++)
		if (myttys[i].mtt_flag & TTY_ON)
			myttys[i].mtt_pid = start_getty(myttys[i].mtt_name);

	while(shutdown != 1)
	{
		while ((pid = wait4tty()) == -1)
		{
			if (shutdown == 1)
				return;
		}

		if (slowdown != 1)
			for (i = 0; i < num_of_ents; i++)
				if (myttys[i].mtt_flag & TTY_ON)
					if (myttys[i].mtt_pid == pid)
					{
						write_utmp(myttys[i].mtt_name, "\0", "\0", time(0L));
						write_wtmp(myttys[i].mtt_name, "\0", "\0", time(0L));
#ifdef DEBUG
						fprintf(console, "init: run_ttys: Starting tty %s.\n", myttys[i].mtt_name);
#endif
						myttys[i].mtt_pid = start_getty(myttys[i].mtt_name);
					}
	}
}

wait4tty()
{
	int pid;
	union wait status;
	struct rusage usage;

	while (1)
	{
		pid = wait3(&status, 0, &usage);
		if ((!WIFSTOPPED(status)) || (pid == -1))
			return pid;
	}
}

void do_single_user()
{
	register int i;
	
	shutdown = 1;
	slowdown = 0;

	flags = SINGLE_USER;

	do_killprocs();

	times_up = 0;
}

void cease_new_ttys()
{
	slowdown = 1;
}

void do_shutdown()
{
	shutdown = 1;
	slowdown = 0;

	flags = HALT;

	do_killprocs();

	fprintf(console, "Syncing file systems...");
	fflush(console);

#ifdef SYNC_FILESYS
	sync();
	sync();
	sync();

	sleep(1);
#endif

	fprintf(console, "done.\n");
	exit(0);
}

do_killprocs()
{
	register int i, j, doit, waitpid, wontdie = 0;
	static union wait status;
	static struct rusage usage;
#ifdef MINT
	int pid, deadpid;
	DIR *procdir;
	struct direct *entry;
	char *pointer;
	int num_procs;
	int first_time = 1;
#endif

	alarm(30);

	for (i = 0; i <= num_of_ents; i++)
		if (myttys[i].mtt_pid != 0)
		{
			myttys[i].mtt_flag = 0;
			killpg(myttys[i].mtt_pid, SIGHUP);
			for (j = 0, waitpid = 0; (j < 20) && (waitpid == 0); j++)
			{
				waitpid = wait3(&status, 1, &usage);
			}
			if ((waitpid == 0) && (j >= 19))
				killpg(myttys[i].mtt_pid, SIGQUIT);
		}

#ifdef MINT

	do
	{
		num_procs = 0;
		if ((procdir = opendir("u:/proc")) == NULL)
		{
			perror("opendir: u:/proc");
		}
		else
		{
			while ((entry = readdir(procdir)) != NULL)
			{
				char statf[0x80] = "u:/proc/";
				struct stat st;

				entry->d_name[entry->d_namlen] = '\0';
				pointer = strchr(entry->d_name, '.');
				sscanf(++pointer, "%3d", &pid);
				if (!updatepid && pid < getpid() && !strncmp
				    (entry->d_name, "update.", pointer-1-entry->d_name))
					updatepid = pid;

				/* don't wait() for update or TSRs...	-nox */
				strcpy (statf+sizeof "u:/proc", entry->d_name);
				if ((pid != getpid()) && (pid != 0) &&
				    (pid != updatepid) && !stat (statf, &st) &&
				    !(st.st_attr & FA_HIDDEN))
				{
					num_procs++;
					switch (first_time)
					{
					case 0:
					case 1:
						kill(pid, SIGHUP);
						wait(NULL);
						break;
					case 2:
					case 3:
					case 4:
						kill(pid, SIGTERM);
						wait(NULL);
						break;
					default:
						kill(pid, SIGKILL);
						wait(NULL);
					}
				}
			}
			closedir(procdir);
		}
		sleep(5);
		first_time++;
	}
	while ((num_procs > 0) && (times_up == 0));
#endif

	if (times_up > 0)
		fprintf(console, "Warning: Some process(es) wouldn't die.\n");
}

static char ttyenv[256];

start_getty(name)
char *name;
{
	struct ttyent *entry;

	setttyent();

	if ((entry = getttynam(name)) == NULL)
		return;

	endttyent();

	return start_getty_entry(entry, name);
}

start_getty_entry(entry, name)
struct ttyent *entry;
char *name;
{
	char ttyname[32];
	char **argv, **envp;
	int pid, tty;
	char *getty_cmd, *args[256];
	char *parse_getty();

#ifdef NEED_TRANSLATED_CONSOLE
	if (!strcmp(name, "console"))
	{
		sprintf(ttyname, "/dev/con");
	}
	else
#endif
		sprintf(ttyname, "/dev/%s", name);

	/* tesche: first look if this terminal really exists */

	if (access(ttyname, F_OK))
		return 0;

	chown(ttyname, 0, 0);
	chmod(ttyname, 0666);

	getty_cmd = parse_getty(entry, args);

	if ((pid = vfork()) == 0)
	{
		setpgrp(getpid(), getpid());

		tty = open(ttyname, O_RDWR);

#ifdef MINT
		dup2(tty, -1);		/* set controlling terminal */
#endif
		dup2(tty, 0);
		dup2(tty, 1);
		dup2(tty, 2);

		close(tty);

		execv(getty_cmd, args);

		perror("init");
		_exit(0);
	}

	return pid;
}

char *parse_getty(entry, argv)
struct ttyent *entry;
char *argv[];
{
	char *pointer;
	int i = 1;

	pointer = entry->ty_getty;

	argv[0] = pointer;
	
	for (; *pointer != '\0'; pointer++)
		if (*pointer == ' ')
		{
			*pointer = '\0';
			argv[i++] = pointer + 1L;
		}

	argv[i] = NULL;
#ifdef environment
	envp[0] = entry->ty_type;
	envp[1] = NULL;
#endif
	return entry->ty_getty;
}

void generic(int signum)
{
	fprintf(console, "init: Signal no. %d caught.\n", signum);
}

void handle_sigalrm(int signum)
{
	if (shutdown)
		times_up = 1;
}
