/*
 * virtual terminals for MiNT, v0.8 (beta)
 *
 * ttyv1..9 are fast hardware-scrolling text-terminals, ttyv0 is the
 * original console and may still be used for graphic display and GEM.
 * to select terminals hit alt-function key (f1 == ttyv1... f10 == ttyv0),
 * alt-undo kills a terminals key buffer. (useful when your auto-repeat
 * is faster than a program processes input, etc)
 *
 * how it works:  each open ttyv? device has a screen buffer and input pipe.
 * keyboard input is read and scanned for control and alt-f keys by a
 * daemon (in main()) that changes screens and sends input and signals
 * to the ttys processes.  ttyv0 uses the original screen memory,
 * ttyv1..9 use one double-length screen in a `simple' video mode for
 * fast display where possible and store the other ttys screens in line
 * buffers that can be `hardware-scrolled' without taking double memory.
 * unused `stored' terminals take up no screen memory.
 *
 * Comments-To: Juergen Lock <nox@jelal.north.de>
 */

#include <osbind.h>
#include <sysvars.h>
#include <stddef.h>
#undef flock
#undef	_sysbase
#define	_sysbase (* ((OSHEADER **) 0x4f2))
#include <mintbind.h>
#include <setjmp.h>
#include <support.h>
#include <signal.h>
#include <stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wait.h>
#include <utmp.h>
#include "vcon.h"
#include "vtdev.h"

#ifndef O_NOCTTY
#define O_NOCTTY	0x4000
#endif

long _stksize = 0x4000;

/* kernel information */
struct kerinfo *kernel;

struct dev_descr devinfo[] = {
	{&vcon_device, 0, O_TTY, &ttys[0].tt},	/* ttyv0 (console) */
	{&vcon_device, 1, O_TTY, &ttys[1].tt},	/* ttyv1 */
	{&vcon_device, 2, O_TTY, &ttys[2].tt},	/* ttyv2 */
	{&vcon_device, 3, O_TTY, &ttys[3].tt},	/* ttyv3 */
	{&vcon_device, 4, O_TTY, &ttys[4].tt},	/* ttyv4 */
	{&vcon_device, 5, O_TTY, &ttys[5].tt},	/* ttyv5 */
	{&vcon_device, 6, O_TTY, &ttys[6].tt},	/* ttyv6 */
	{&vcon_device, 7, O_TTY, &ttys[7].tt},	/* ttyv7 */
	{&vcon_device, 8, O_TTY, &ttys[8].tt},	/* ttyv8 */
	{&vcon_device, 9, O_TTY, &ttys[9].tt}	/* ttyv9 */
};

#define MAX_VT ((sizeof devinfo)/sizeof (struct dev_descr))

struct ttyv ttys[MAX_VT];

struct sgttyb con;
int conflags;
short hardscroll = -1, os_version;
struct tchars con_tc, tc0;
struct ltchars con_ltc, ltc0;

int pfd[MAX_VT], cfd, vcurrent;
long pgrp;
short leaving;

void con_raw()
{
	Fcntl(0, &con, TIOCGETP);
	Fcntl(0, &con_tc, TIOCGETC);
	Fcntl(0, &con_ltc, TIOCGLTC);
	conflags = con.sg_flags;
	con.sg_flags &= ~(CBREAK|ECHO|TOSTOP);
	Fcntl(0, &con, TIOCSETN);
	Fcntl(0, &tc0, TIOCSETC);
	Fcntl(0, &ltc0, TIOCSLTC);
}

void con_sane()
{
	int i;

	if (cfd >= 0) {
		/* try to uninstall gracefully... */
		int opencnt = 0;
		char *ttyv0name, *oldcname, *s;
		long vpgrp;

		/* /dev/ttyv0 might be renamed /dev/console... */
		if ((ttyv0name=ttyname(cfd)) && (s = strrchr (ttyv0name, '/')) &&
		    !strcmp (s, "/console"))
			rename (ttyv0name, "u:/dev/ttyv0");
		/* move original console device in place */
		if ((oldcname=ttyname(0)) && (s = strrchr (oldcname, '/')) &&
		    strcmp (s, "/console"))
			rename (oldcname, "u:/dev/console");

		/* tell processes their tty is going away */
		leaving = 1;
		for (i = 0; i < MAX_VT; ++i) {
			if (ttys[i].tt.use_cnt > !i) {
				++opencnt;
				if ((vpgrp = ttys[i].tt.pgrp) && vpgrp != pgrp)
					killpg(vpgrp, SIGHUP);
			}
		}
		/* we can't unlink devices before they are all closed...
		   this can cause deadlocks but what can i do? :(
		*/
		while (opencnt) {
			opencnt = 0;
			/* sleep(1);  (save space...) */
			(void) Fselect (1000, 0l, 0l, 0l);
			for (i = 0; i < MAX_VT; ++i)
				if (ttys[i].tt.use_cnt > !i)
					++opencnt;
		}
		Fcntl(cfd, (char *) 0, VCTLSETV);
		close (cfd);
		cfd = -1;
#if 1
		vpgrp = 0;
		Fcntl(0, &vpgrp, TIOCSPGRP);
#endif
	}
	/* unlink devices */
	for (i = 0; i < MAX_VT; ++i) {
		char name[] = "u:\\dev\\ttyv0";

		name[sizeof "u:\\dev\\ttyv"-1] = i+'0';
		Fdelete (name);
	}

	con.sg_flags=conflags;	/* restore console  */
	Fcntl(0, &con, TIOCSETN);
	Fcntl(0, &con_tc, TIOCSETC);
	Fcntl(0, &con_ltc, TIOCSLTC);
}

/* handle fatal signals */
void trap(sig)
int sig;
{
	if (leaving)
		return;
	con_sane();
	signal(sig, SIG_DFL);
	/* die! */
	kill(getpid(), sig);
}

/* pass signal to process on the terminal */
void trap_int(sig)
int sig;
{
	int vpgrp;

	/* see who is on the tty and if its a different process group... */
	if (ttys[vcurrent].tt.use_cnt && ((vpgrp = ttys[vcurrent].tt.pgrp)) &&
	    vpgrp != pgrp) {
		/* if yes, pass the signal */
		signal(sig, SIG_IGN);
		killpg(vpgrp, sig);
		signal(sig, trap_int);
	}
}

/* SIGTTIN/OU */
void trap_tt(sig)
int sig;
{
	long vpgrp = 0;

	Fcntl(0, &vpgrp, TIOCGPGRP);
	if (vpgrp && vpgrp != pgrp) {
		/* huh!?  someone stole our console device... */
		signal(sig, SIG_IGN);
		killpg(vpgrp, sig);
		signal(sig, trap_tt);
	}
	Fcntl(0, &pgrp, TIOCSPGRP);
}

static	OSHEADER *syshdr;

void getsyshd()
{
	syshdr = _sysbase->os_beg;
}

/* get a (TOS version dependant) pointer to the current shift/alt
   keyboard status
*/
char *getpkbshift()
{
	/* get OS header */
	(void) Supexec(getsyshd);
	/* TOS 1.(0)2 or newer has it in the header */
	if ((os_version = syshdr->os_version) > 0x100)
		return (char *)syshdr->pkbshift;
	else
	/* TOS 1.0 */
		return (char *) 0x0e1bL;
}

#ifndef HAVE_REAL_FORK
/* on MiNT fork and vfork both block until the child does either exec or
   dies.  only tfork doesn't block but it works like a subroutine call... */
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
#endif

/* queue up characters for a terminal */

void csend (vcurrent, fd, cbuf, bufp)
int vcurrent, fd;
long *cbuf, *bufp;
{
	long bytes = (char *)bufp - (char *)cbuf;

	if (bytes) {
		/* closed/pipe full? */
		if (!ttys[vcurrent].tt.use_cnt || Foutstat (fd) < bytes)
			/* bing... */
			Fputchar (0, 07l, 0);
		else {
			/* else send buffer */
			Fwrite (fd, bytes, cbuf);
			/* if someone select()ed this terminal wake 'em up */
			if (ttys[vcurrent].tt.rsel)
				Fcntl(cfd, (char *) (long) vcurrent, VCTLWSEL);
		}
	}
}

#ifdef TGETTY
/* spawn getty (once) on ttyv(t), to use compile with something like
 *	'-DTGETTY="/bin/runtt", "runtt", "-t", tty, "/etc/getty", "vty"'
 * (see Makefile)
 * the child should return ASAP, with return code 0 if successful
 */
int dogetty(t)
int t;
{
	char tty[] = "/dev/ttyv0";
	char prompt[] = "ttyv0 login: ";
	int status;

	prompt[sizeof "ttyv"-1] = tty[sizeof "/dev/ttyv"-1] = t+'0';
	switch (vfork()) {		/* ok since runtt will fork itself */
	case -1:
		status = -1;		/* fork failed */
		break;
	case 0:
		/* child */
		for (status = 0; status < 3; ++status)
		/* we don't want it mess with our /dev/console... */
			Fclose(status);
		execl (TGETTY, (char *) 0);  /* has to open the tty itself */
		/* exec failed */
		_exit(1);
		/*NOTREACHED*/
	default:
		/* parent */
		if (wait(&status) < 0)
			status = -1;
	}
	return status;
}

#ifdef TGETTYUTMP
#define UTMP_FILE	"/etc/utmp"
#define WTMP_FILE	"/var/adm/wtmp"

static enum gettystate {TNONE, TFORKING, TOPEN} tgstate[MAX_VT];

int zaputmp(t)
int t;
{
	char tty[] = "ttyv0";
	static struct utmp wentry;
	struct utmp rentry;
	int i, r, fd;
	int open();

	tty[sizeof "ttyv"-1] = t+'0';
	strncpy(wentry.ut_line, tty, 8);
	wentry.ut_time = time(0L);

	bzero(&rentry, sizeof(struct utmp));

	if ((fd = open(UTMP_FILE, O_RDWR)) == -1)
		return 1;
	for (i = 0; ((r = read(fd, &rentry, sizeof(rentry))) != -1); ++i) {
		if (!r)
			return 1;
		if (!strncmp(tty, rentry.ut_line, 8))
			break;
	}
	if (lseek(fd, (i * sizeof(rentry)), 0) == -1 ||
	    write(fd, &wentry, sizeof(wentry)) != sizeof(wentry)) {
		close(fd);
		return 1;
	}
	close(fd);
	if ((fd = open(WTMP_FILE, (O_RDWR | O_APPEND))) == -1)
		return 1;
	if (write(fd, &wentry, sizeof(wentry)) != sizeof(wentry)) {
		close(fd);
		return 1;
	}
	close(fd);
	return 0;
}
#endif
#endif

/*
 * main:  initialization stuff, and a daemon that handles input from
 * the physical console
 */

main()
{
	int i;
	volatile char *pkbshift = getpkbshift();
	long cbuf[0x80], *bufp;
	extern int __mint;
	int open();

#if 0
	/* sanity check */
	if (!(s = ttyname (0)) || (!(s = strrchr (s, '/'))) ||
	    strcmp (s, "/console")) {
		Cconws ("Sorry this is fast not `clean' software :)  console only...\r\n");
		exit (1);
	}
#endif
	if (__mint < 9) {
		Cconws ("Sorry MiNT kernel version too old, need O_NDELAY...\r\n");
		exit (1);
	}
	/* open console ourselves to be sure noone clears our O_NDELAY flag */
	if ((i = open ("/dev/console", O_RDWR|O_NDELAY)) < 0 ||
	    dup2 (i, 0) < 0 || dup2 (i, 1) < 0 || dup2 (i, 2) < 0) {
		Cconws ("Huh?  unable to open /dev/console...\r\n");
		exit (1);
	}
	for (i = 3; i < 32; ++i) {
		close (i);
	}

	/* stdin RAW, catch signals...  */
	pgrp = getpid(/*0*/);
	Fcntl(0, &con, TIOCGETP);
	Fcntl(0, &con_tc, TIOCGETC);
	Fcntl(0, &con_ltc, TIOCGLTC);
	conflags = con.sg_flags;
	signal(SIGHUP, trap);
	signal(SIGTERM, trap);
	signal(SIGINT, trap_int);
	signal(SIGQUIT, trap_int);
	signal(SIGTSTP, trap_int);
	signal(SIGTTIN, trap_tt);
	signal(SIGTTOU, trap_tt);
	con_raw();

	umask (022);
	for (i = 0; i < MAX_VT; ++i) {
		char name[] = "u:\\pipe\\q$ttyv0";

		name[sizeof "u:\\pipe\\q$ttyv"-1] = i+'0';
		if ((pfd[i] = Fcreate (name, FA_RDONLY|FA_CHANGED)) < 0) {
			con_sane();
			Cconws ("Huh?  unable to create pipe...\r\n");
			exit (1);
		}
		/* hmm.  tfork goes thru Pexec, and Pexec looks at
		   close-on-exec flags...
		*/
		Fcntl (pfd[i], 0, F_SETFD);
	}
	for (i = 0; i < MAX_VT; ++i) {
		char name[] = "u:\\dev\\ttyv0", *p = ttys[i].readxlat;
		int j;

		/* init input translation == none */
		for (j = 0x80; j < 0x100; ++j) {
			*p++ = j;
		}
		name[sizeof "u:\\dev\\ttyv"-1] = i+'0';
		if (vcon_device.writeb)
#ifndef follow_links	/* filesys.h */
			devinfo[i].devdrvsiz = offsetof (DEVDRV, writeb) + sizeof (long);
#else			/* kernel file.h... */
			devinfo[i].drvsize = offsetof (DEVDRV, writeb) + sizeof (long);
#endif
		kernel = (struct kerinfo *)Dcntl(DEV_INSTALL, name, devinfo+i);
		if (!kernel || ((long)kernel) == -32) {
			con_sane();
			Cconws ("Uh :(  unable to install vt device(s)!\r\n");
			exit (1);
		}
	}

	if ((cfd = open ("u:\\dev\\ttyv0", O_NOCTTY|O_RDWR)) < 0) {
		con_sane();
		Cconws ("Help!!  open new console device failed.\r\n");
		exit (1);
	}
#if 0
	/* better do this in rc.local... (or mint.cnf) */
	Frename (0, "u:\\dev\\console", "u:\\dev\\con00");
	Frename (0, "u:\\dev\\ttyv0", "u:\\dev\\console");
#endif

#ifndef HAVE_REAL_FORK
	/* hack until MiNT gets a real nonblocking fork...  one day :)
	*/
	if (!setjmp(tforkj) && tfork (in_tfork, 0l) >= 0)
		_exit (0);
#else
	if (fork())
		_exit (0);
#endif

	/* ok parent continues, should not read old /dev/console anymore now */
	pgrp = setpgrp(/*pgrp, pgrp*/);
	Fcntl(0, &pgrp, TIOCSPGRP);
	/* hmm part 2: tfork resets signals too... */
	signal(SIGHUP, trap);
	signal(SIGTERM, trap);
	signal(SIGINT, trap_int);
	signal(SIGQUIT, trap_int);
	signal(SIGTSTP, trap_int);
	signal(SIGTTIN, trap_tt);
	signal(SIGTTOU, trap_tt);

	/* now the daemon part
	   poll keyboard, switch between terminals, start/stop output,
	   send signals, flash cursor...
	*/
	bufp = cbuf;
	for (;;) {
		char name[] = "u:\\pipe\\q$ttyv0";
		int fd = pfd[vcurrent], scan;
		char cshift;
		unsigned long l;
		static fshort = 0;

		/* empty buffer if full */
		if (bufp == cbuf+sizeof cbuf) {
			csend (vcurrent, fd, cbuf, bufp);
			bufp = cbuf;
		}
		cshift = *pkbshift;
		/* if nothing to read empty buffer and do a select */
		if ((l=Fgetchar (0, 0)) == MiNTEOF) {
			csend (vcurrent, fd, cbuf, bufp);
			bufp = cbuf;
			for (;;) {
				SCREEN *v = v0x+vcurrent-1;
				int ctimeout = 500;
				long rfd = 1;

				if (vcurrent && (CURS_FLASH|CURS_ON) ==
				    (v->flags & (CURS_FLASH|CURS_ON))) {
					ctimeout = 20*(unsigned char)v->period;
					if (!ctimeout)
						ctimeout = 20*0x100;
					if (fshort) {
						ctimeout /= 2;
						ctimeout += ctimeout/4;
					}
				}
				fshort = 0;

				/* BUG: (MiNT 1.09)  select doesn't wake up for
				   `repeated' keys...  not until timeout or you
				   release the key.  */
				if (Fselect (ctimeout, &rfd, (long *)0, (long *)0)) {
					/* show cursor if flashing and off */
					if (vcurrent && (CURS_FLASH|CURS_ON) ==
					    (v->flags & (CURS_FLASH|CURS_ON|CURS_FSTATE))) {
						Fcntl(cfd, (char *) 0, VCTLFLASH);
					}
					/* then go read whats there */
					break;
				}
				/* select timed out, flash cursor & try again */
				Fcntl(cfd, (char *) 0, VCTLFLASH);
#ifdef TGETTYUTMP
				for (i = 1; i < MAX_VT; ++i) {
					switch (tgstate[i]) {
					case TFORKING:
						if (ttys[i].tt.use_cnt)
/* getty is up... */
							tgstate[i] = TOPEN;
						break;
					case TOPEN:
						if (!ttys[i].tt.use_cnt) {
/* we put the getty there, we have to clean up after it */
							zaputmp(i);
							tgstate[i] = TNONE;
						}
						break;
					default:
					}
				}
#endif
			}
			continue;
		}
		if ((cshift & ~0x10) == 8)  switch (scan=(char) (l>>16)) {
			int xfd;	/* ALT-something */

			case 0x61:	/* ALT-UNDO: flush pipe if !empty */
				name[sizeof "u:\\pipe\\q$ttyv"-1] = vcurrent+'0';
				if ((xfd = Fopen (name, O_RDONLY)) >= 0
				    && Finstat (xfd) > 0) {
					Fcntl (xfd, (char *) 0, TIOCFLUSH);
					Fclose (xfd);
					bufp = cbuf;
					continue;
				} else if (bufp > cbuf) {
					bufp = cbuf;
					continue;
				}
				break;
			case 0x3b:	/* ALT-F1: ttyv1 */
			case 0x3c:	/* ALT-F2: ttyv2 */
			case 0x3d:	/* .	*/
			case 0x3e:	/* .	*/
			case 0x3f:	/* .	*/
			case 0x40:
			case 0x41:
			case 0x42:
			case 0x43:	/* ALT-F9:  ttyv9 */
				scan += 10;
				/*FALLTHRU*/
			case 0x44:	/* ALT-F10: ttyv0 */
				if ((scan -= 0x44) < MAX_VT) {
					csend (vcurrent, fd, cbuf, bufp);
					bufp = cbuf;
					if (Fcntl(cfd, (char *) (long) scan, VCTLSETV))
						Fputchar (0, 07l, 0);
					else
						fshort = 1;
					continue;
				}
			break;
#ifdef TGETTY
		} else	if ((cshift & ~0x10) == 0xc)  switch (scan=(char) (l>>16)) {
			int i;
			case 0x63:	/* ctrl-alt-num( */
				csend (vcurrent, fd, cbuf, bufp);
				bufp = cbuf;
				for (i = 1; i < MAX_VT; ++i) {
					if (!ttys[i].tt.use_cnt) {
/* find a free tty and put up a getty...
 * if just now something else decides to use this ttyv you'll lose.
 */
#ifdef TGETTYUTMP
						if (tgstate[i]) continue;
						tgstate[i] = TFORKING;
#endif
						if (!dogetty(i) &&
						    !Fcntl(cfd, (char *) (long) i, VCTLSETV)) {
							fshort = 1;
							goto ok;
						}
						break;
					}
				}
				Fputchar (0, 07l, 0);
ok:
				continue;
#endif
		} else	if ((ttys[vcurrent].tt.state & TS_COOKED) ||
			    (cshift & 0xc) == 0xc) {
			char ch = (char) l;
			int xfd = cfd, sig = 0;
			char xname[] = "u:\\dev\\ttyv0";

			xname[sizeof "u:\\dev\\ttyv"-1] = vcurrent+'0';
			if (!ch)
				;	/* do nothing */
			else if (ch == ttys[vcurrent].tt.tc.t_intrc)
				sig = SIGINT;
			else if (ch == ttys[vcurrent].tt.tc.t_quitc)
				sig = SIGQUIT;
			else if (ch == ttys[vcurrent].tt.ltc.t_suspc)
				sig = SIGTSTP;
			else if (ch == ttys[vcurrent].tt.tc.t_stopc) {
				if (!vcurrent ||
				    (xfd = Fopen (xname, O_RDONLY)) >= 0) {
					Fcntl (xfd, (char *) 0, TIOCSTOP);
					if (vcurrent)
						Fclose (xfd);
				}
				continue;
			}
			else if (ch == ttys[vcurrent].tt.tc.t_startc) {
				if (!vcurrent ||
				    (xfd = Fopen (xname, O_RDONLY)) >= 0) {
					Fcntl (xfd, (char *) 0, TIOCSTART);
					if (vcurrent)
						Fclose (xfd);
				}
				continue;
			}
			if (sig) {
				if (!vcurrent ||
				    (xfd = Fopen (xname, O_RDONLY)) >= 0) {
					Fcntl (xfd, (char *) 0, TIOCSTART);
					if (vcurrent)
						Fclose (xfd);
				}
				if (!(ttys[vcurrent].tt.sg.sg_flags & T_NOFLSH))
					Fcntl (fd, (char *) 0, TIOCFLUSH);
				else
					csend (vcurrent, fd, cbuf, bufp);
				bufp = cbuf;
				if (ttys[vcurrent].tt.pgrp)
					killpg (ttys[vcurrent].tt.pgrp, sig);
				continue;
			}
			else if (ttys[vcurrent].tt.state & TS_HOLD) {
				continue;
			}
		}
		if ((unsigned char)l > 0x7f)
			l = (l & ~0xff) | ttys[vcurrent].readxlat[l & 0x7f];
		*bufp++ = l;
	}
}

