/* runtt -- poor man's init :) for MiNT
   open tty, setpgrp(), exec command on it...

   usage: runtt command >tty
      or: runtt -T tty command  (empty command gets loginshell)
      or: runtt -t tty command  (like -T but (t)forks before.)

~Subject: put shells etc. on (virtual) tty without init...
~To: mint@terminator.rs.itd.umich.edu
~Date: Wed, 29 Sep 93 23:42:26 CET
~From: Juergen Lock <nox@jelal.north.de>
~X-Mailer: ELM [version 2.3/ST PL11]
~Message-Id: <9309292242.AA00164@jelal.north.de>

here's a little hack that puts a command or shell on a terminal.  useful
when you don't have enough RAM for init _and_ gcc...  (or when you don't
like init for some other reason though i can't think of one :-)

 easiest way, and also a good place in terms of memory fragmentation is
to put it in a mint.cnf:  (otherwise it can be called from anywhere.)

# first start virtual consoles... (of course other ttys should also work :)
exec u:\usr\etc\vcons1d
ren u:\dev\console u:\dev\con0
ren u:\dev\ttyv0 u:\dev\console
CON=u:\dev\console
# turn on cursor (make the ^[ one esc char)
echo ^[e
. 
. 
# set $SHELL (default is /bin/sh)
setenv SHELL u:\bin\ksh.ttp
# (and also $HOME, $TERM...)
. 
. 
cd u:\home\nox
# init for console (could also be GEM... i prefer to start it from the
# shell when i need it)
INIT=u:\bin\ksh.ttp -L

# everything set, now put a top on ttyv1...
exec u:\local\bin\runtt.ttp -t ttyv1 top
# and 2 (more) shells on ttyv2 and 3
exec u:\local\bin\runtt.ttp -t ttyv2
exec u:\local\bin\runtt.ttp -t ttyv3
# maybe add a sleep here so the shells have time to come up before GEM.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <support.h>
#include <mintbind.h>
#include <errno.h>

static jmp_buf	tforkj;

static int in_tfork(arg)
int arg;
{
	/* wait for parent to die before we can longjmp back */
	while (getppid () > 1)
		(void) Fselect (1000, 0l, 0l, 0l);
	longjmp (tforkj, 1);
	/*NOTREACHED*/
}

static jmp_buf stopj;

void ttout(sig)
int sig;
{
	longjmp (stopj, 1);
}

int main(argc, argv)
int argc;
char **argv;
{
	long pgrp = setpgrp(/*getpid(), getpid()*/);
	static char *name, buf[0x100] = "/dev/", **xargv;
	static int tty, do_fork = 0;
	char *shell;

	xargv = argv;
	if (argv[1] && argv[1][0] == '-' && (argv[1][1] | 0x20) == 't'
	    && !argv[1][2] && (name = argv[2])) {
		if (*name != '/' && *name != '\\' &&
		    (!*name || name[1] != ':')) {
			strcat (buf, name);
			name = buf;
		}
		if (argv[1][1] == 't')
			do_fork = 1;
		tty = open(name, O_RDWR);
		xargv += 2;
	} else {
		name = ttyname(1);
		tty = dup(1);
	}

	if (tty == -1 || pgrp < 0) {
		signal (SIGTTOU, SIG_IGN);
		perror(name);
		return 1;
	}
	/* tty in use?  stat doesn't tell on MiNT, so... */
	signal (SIGTTOU, ttout);
	if (_isctty(tty) || setjmp (stopj)) {
		signal (SIGTTOU, SIG_IGN);
		fprintf(stderr, "%s: tty in use\n", name);
		return 1;
	}
	/* write NUL to trigger SIGTTOU... */
	Fputchar (tty, 0, 0);
	/* if we get here there was no sig */
	if (do_fork) {
		Fcntl (tty, 0, F_SETFD);
		if (!setjmp(tforkj) && tfork (in_tfork, 0) >= 0)
			_exit (0);
	}
	dup2(tty, -1);
	ioctl (-1, TIOCSPGRP, &pgrp);

	dup2(tty, 0);
	dup2(tty, 1);
	dup2(tty, 2);

	if (tty > 2) close(tty);

	shell = getenv("SHELL");
	if (!shell)
		shell = "/bin/sh";
	if (!xargv[1] ||
	    (execvp((name = xargv[1]), xargv+1) < 0 &&
	     (errno == ENOEXEC || errno == ENOENT))) {
		name = shell;

		/* loginshell? */
		if (!xargv[1]) {
			char *p, *q;

			for (p = shell; (q = strpbrk(p, "/\\:"));)
				p = ++q;
			xargv[0] = q = buf;
			*q++ = '-';
			strcpy (q, p);
		} else
			xargv[0] = shell;
		execv(shell, xargv);
	}
	perror(name);
	return 1;
}
