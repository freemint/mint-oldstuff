/*
 * virtual terminals for MiNT, v0.2 (still alpha...)
 *
 * vt01..9 are fast hardware-scrolling text-terminals, vt00 is the
 * original console and can still be used for GEM. (i hope :)
 * to change terminals hit alt-function key (f1 == vt01... f10 == vt00),
 * alt-undo kills a terminals key buffer. (useful when your auto-repeat
 * is faster than a program processes input, etc)
 *
 * how it works:  each open vt.. device has a screen buffer and input pipe.
 * keyboard input is read and scanned for control and alt-f keys by a
 * daemon (in main()) that changes screens and sends input and signals to
 * the correct processes.
 * vt00 uses the original screen memory, vt01..9 share one bigger screen
 * for display and store the other screens contents in line buffers
 * so they can be `hardware-scrolled' without taking extra memory.
 * (just move pointers around.. line offsets actually)  closed `stored'
 * terminals don't take up screen memory.
 *
 * send bugs & comments to:  Juergen Lock <nox@jelal.north.de>
 */

#include <osbind.h>
#include <sysvars.h>
#undef flock
#undef	_sysbase
#define	_sysbase (* ((OSHEADER **) 0x4f2))
#include <mintbind.h>
#include <setjmp.h>
#include <support.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vcon.h"
#include "vtdev.h"

/* kernel information */
struct kerinfo *kernel;

struct dev_descr devinfo[] = {
	{&vcon_device, 0, O_TTY, ttys+0, {0L, 0L, 0L, 0L}},  /* vt00 (console) */
	{&vcon_device, 1, O_TTY, ttys+1, {0L, 0L, 0L, 0L}},  /* vt01 */
	{&vcon_device, 2, O_TTY, ttys+2, {0L, 0L, 0L, 0L}},  /* vt02 */
	{&vcon_device, 3, O_TTY, ttys+3, {0L, 0L, 0L, 0L}},  /* vt03 */
	{&vcon_device, 4, O_TTY, ttys+4, {0L, 0L, 0L, 0L}},  /* vt04 */
	{&vcon_device, 5, O_TTY, ttys+5, {0L, 0L, 0L, 0L}},  /* vt05 */
	{&vcon_device, 6, O_TTY, ttys+6, {0L, 0L, 0L, 0L}},  /* vt06 */
	{&vcon_device, 7, O_TTY, ttys+7, {0L, 0L, 0L, 0L}},  /* vt07 */
	{&vcon_device, 8, O_TTY, ttys+8, {0L, 0L, 0L, 0L}},  /* vt08 */
	{&vcon_device, 9, O_TTY, ttys+9, {0L, 0L, 0L, 0L}}   /* vt09 */
};

#define MAX_VT ((sizeof devinfo)/sizeof (struct dev_descr))

struct tty ttys[MAX_VT];

struct sgttyb con;
int conflags;
short hardscroll = -1;
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
		char *vt00name=ttyname(cfd), *oldcname=ttyname(0), *s;
		long vpgrp;

		/* /dev/vt00 might be renamed /dev/console... */
		if (vt00name && (s = strrchr (vt00name, '/')) &&
		    !strcmp (s, "/console"))
			rename (vt00name, "u:/dev/vt00");
		/* move original console device in place */
		if (oldcname && (s = strrchr (oldcname, '/')) &&
		    strcmp (s, "/console"))
			rename (oldcname, "u:/dev/console");

		/* tell processes their tty is going away */
		leaving = 1;
		for (i = 0; i < MAX_VT; ++i) {
			if (ttys[i].use_cnt > !i) {
				++opencnt;
				if ((vpgrp = ttys[vcurrent].pgrp))
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
				if (ttys[i].use_cnt > !i)
					++opencnt;
		}
		Fcntl(cfd, (char *) 0, VCTLSETV);
		Fclose (cfd);
#if 1
		vpgrp = 0;
		Fcntl(0, &vpgrp, TIOCSPGRP);
#endif
	}
	/* unlink devices */
	for (i = 0; i < MAX_VT; ++i) {
		char name[] = "u:\\dev\\vt00";

		name[sizeof "u:\\dev\\vt0"-1] = i+'0';
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
	if (ttys[vcurrent].use_cnt && ((vpgrp = ttys[vcurrent].pgrp)) &&
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
	if (syshdr->os_version > 0x100)
		return syshdr->pkbshift;
	else
	/* TOS 1.0 */
		return (char *) 0x0e1bL;
}

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

/* queue up characters for a terminal */

void csend (vcurrent, fd, cbuf, bufp)
int vcurrent, fd;
long *cbuf, *bufp;
{
	long bytes = (char *)bufp - (char *)cbuf;

	if (bytes) {
		/* pipe full? */
		if (Foutstat (fd) < bytes)
			/* bing... */
			Fputchar (0, 07l, 0);
		else {
			/* else send buffer */
			Fwrite (fd, bytes, cbuf);
			/* if someone select()ed this terminal wake 'em up */
			if (ttys[vcurrent].rsel)
				Fcntl(cfd, (char *) vcurrent, VCTLWSEL);
		}
	}
}

/*
 * main:  initialization stuff, and a daemon that handles input from
 * the physical console
 */

main()
{
	int i;
	char *s;
	volatile char *pkbshift = getpkbshift();
	long cbuf[0x80], *bufp;

	/* sanity check */
	if (!(s = ttyname (0)) || (!(s = strrchr (s, '/'))) ||
	    strcmp (s, "/console")) {
		Cconws ("Sorry this is fast not `clean' software :)  console only...\r\n");
		exit (1);
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

	for (i = 0; i < MAX_VT; ++i) {
		char name[] = "u:\\pipe\\q$vt00";

		name[sizeof "u:\\pipe\\q$vt0"-1] = i+'0';
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
		char name[] = "u:\\dev\\vt00";

		name[sizeof "u:\\dev\\vt0"-1] = i+'0';
		kernel = (struct kerinfo *)Dcntl(DEV_INSTALL, name, devinfo+i);
		if (!kernel || ((long)kernel) == -32) {
			con_sane();
			Cconws ("Uh :(  unable to install vt device(s)!\r\n");
			exit (1);
		}
	}

	if ((cfd = Fopen ("u:\\dev\\vt00", O_RDWR)) < 0) {
		con_sane();
		Cconws ("Help!!  Fopen new console device failed.\r\n");
		exit (1);
	}
	Fcntl (cfd, 0, F_SETFD);
#if 0
	/* better put this in rc.local... (or mint.cnf) */
	Frename (0, "u:\\dev\\console", "u:\\dev\\con00");
	Frename (0, "u:\\dev\\vt00", "u:\\dev\\console");
#endif

	/* hack until MiNT gets a real nonblocking fork...  one day :)
	*/
	if (!setjmp(tforkj) && tfork (in_tfork, 0l) >= 0)
		_exit (0);

	/* ok parent continues, has to close old /dev/console now... */
	pgrp = setpgrp(/*pgrp, pgrp*/);
	Fcntl(0, &pgrp, TIOCSPGRP);
	Fcntl(0, (void *) O_NDELAY, F_SETFL);
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
		char name[] = "u:\\pipe\\q$vt00";
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
			}
			continue;
		}
		if ((cshift & ~0x10) == 8)  switch (scan=(char) (l>>16)) {
			int xfd;	/* ALT-something */

			case 0x61:	/* ALT-UNDO: flush pipe if !empty */
				name[sizeof "u:\\pipe\\q$vt0"-1] = vcurrent+'0';
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
			case 0x3b:	/* ALT-F1: vt01 */
			case 0x3c:	/* ALT-F2: vt02 */
			case 0x3d:	/* .	*/
			case 0x3e:	/* .	*/
			case 0x3f:	/* .	*/
			case 0x40:
			case 0x41:
			case 0x42:
			case 0x43:	/* ALT-F9:  vt09 */
				scan += 10;
				/*FALLTHRU*/
			case 0x44:	/* ALT-F10: vt00 */
				if ((scan -= 0x44) < MAX_VT) {
					csend (vcurrent, fd, cbuf, bufp);
					bufp = cbuf;
					if (Fcntl(cfd, (char *) scan, VCTLSETV))
						Fputchar (0, 07l, 0);
					else
						fshort = 1;
					continue;
				}
			break;
		} else	if ((ttys[vcurrent].state & TS_COOKED) ||
			    (cshift & 0xc) == 0xc) {
			char ch = (char) l;
			int sig = 0;

			if (!ch)
				;	/* do nothing */
			else if (ch == ttys[vcurrent].tc.t_intrc)
				sig = SIGINT;
			else if (ch == ttys[vcurrent].tc.t_quitc)
				sig = SIGQUIT;
			else if (ch == ttys[vcurrent].ltc.t_suspc)
				sig = SIGTSTP;
			else if (ch == ttys[vcurrent].tc.t_stopc) {
				ttys[vcurrent].state |= TS_HOLD;
				continue;
			}
			else if (ch == ttys[vcurrent].tc.t_startc) {
				ttys[vcurrent].state &= ~TS_HOLD;
				continue;
			}
			if (sig) {
				ttys[vcurrent].state &= ~TS_HOLD;
				if (!(ttys[vcurrent].sg.sg_flags & T_NOFLSH))
					Fcntl (fd, (char *) 0, TIOCFLUSH);
				else
					csend (vcurrent, fd, cbuf, bufp);
				bufp = cbuf;
				if (ttys[vcurrent].pgrp)
					killpg (ttys[vcurrent].pgrp, sig);
				continue;
			}
			else if (ttys[vcurrent].state & TS_HOLD) {
				continue;
			}
		}
		*bufp++ = l;
	}
}

