/* execgem -- exec `old' GEM (exec_os) or a gem.sys (-DINITPRG)
   make sure we're on console, zero its pgroup to reduce GEM SIGTTIN/OU
   problems (don't ask), exec.  INITPRG can also be a script so you
   can create one or more logins in /etc/passwd for different GEM
   configurations, give it this as loginshell, and put setup things
   in the script. (last line execs the real GEM.)

   BUGS: still no way to leave GEM & free all its memory and vectors etc.
   other than reboot. :-(  if someone manages to shut down GEM and later
   restart it without spectacular crashes and losing memory _please_
   tell us how...
*/

#include <stdio.h>
#include <string.h>
#include <stat.h>
#include <fcntl.h>
#include <ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <mintbind.h>
#include <basepage.h>
#include <st-out.h>

#ifdef INITPRG
#define init_prg INITPRG
#define init_tail "\0"
#define init_env 0L
#else
#define init_prg 0L
#define init_tail ""
#define init_env 0L
#endif

#define EXEC_OS 0x4feL

/* define how to call functions with stack parameter passing */
#ifdef __TURBOC__
#define ARGS_ON_STACK cdecl
#else
#define ARGS_ON_STACK
#endif

#ifndef P_
# ifdef __STDC__
#  define P_(x) x
# else
#  define P_(x) ()
# endif
#endif

/* not yet in st-out.h... */
#ifndef F_PROT_P
#define F_PROT_P	0x00		/* no read or write */
#define F_PROT_G	0x10		/* any access OK */
#define F_PROT_S	0x20		/* any super access OK */
#define F_PROT_PR	0x30		/* any read OK, no write */
#define F_PROT_I	0x40		/* invalid page */
#endif

#ifndef F_ALLOCZERO
#define F_ALLOCZERO	0x2000		/* zero mem, for bugged (GEM...) programs */
#endif

long GEM_memflags = F_ALLOCZERO | F_ALTLOAD | F_ALTALLOC | F_PROT_G;

int
main(argc, argv)
	int argc;
	char **argv;
{
	long stack;
	extern int __mint;

	if (__mint) {
		struct stat sb, st;
		BASEPAGE *bp;
		long pgrp, r;
		char tmp[0x100];

		if (Fcntl(-1, &pgrp, TIOCGPGRP) ||
		    Fcntl(-1, &sb, FSTAT) ||
		    (Fxattr(0, "u:\\dev\\ttyv0", &st) &&
			Fxattr(0, "u:\\dev\\vt00", &st) &&
			Fxattr(0, "u:\\dev\\console", &st)) ||
		    ((st.st_dev != sb.st_dev || st.st_ino != sb.st_ino) &&
		     (Fxattr(0, "u:\\dev\\fasttext", &st) ||
		        st.st_dev != sb.st_dev || st.st_ino != sb.st_ino))) {

			Cconws ("Hey GEM is not X! :)  console only...\r\n");
			return 1;
		}

		/* just to be sure... */
		(void) Fclose (0);
		(void) Fcntl(-1, 0L, F_DUPFD);
		(void) Fclose (1);
		(void) Fcntl(-1, 0L, F_DUPFD);
		(void) Fclose (2);
		(void) Fcntl(-1, 0L, F_DUPFD);

		/* `turn off' job control on /dev/console, GEM seems to have
		   problems with it...  apparently even multitos :-(  */
		r = 0;
		(void) Fcntl(0, &r, TIOCSPGRP);

		stack = Super (0L);
		if (init_prg) {
			/* not Pexec so init_prg gets ARGV and can be a script.
			*/
			execl (init_prg, init_prg, (char *)0);
			if (errno == ENOEXEC)
				execl ("/bin/sh", "/bin/sh", init_prg, (char *)0);
			r = errno;
		} else {
			bp = (BASEPAGE *)Pexec(7,
			  (char *)GEM_memflags,
			  (char *)"\0", init_env);
			bp->p_tbase = *((char **) EXEC_OS );
			r = -Pexec(206, (char *)"GEM", bp, 0L);
		}
		Super (stack);
		sprintf (tmp, "exec %s failed: %s (%d)\r\n",
			init_prg ? init_prg : "GEM", strerror (r), (int) r);
		(void) Fcntl(0, &pgrp, TIOCSPGRP);
		Cconws (tmp);
		return r;
	} else {
		register void ARGS_ON_STACK (*f) P_((long));

		/* no MiNT -> just jump thru exec_os... */
		stack = Super (0L);
		f = (void ARGS_ON_STACK (*) P_((long))) *((long *)EXEC_OS);
		Super (stack);

		(*f)((long) _base);
		/*NOTREACHED*/
		return 0;
	}
}
