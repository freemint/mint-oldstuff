#include <ostruct.h>

#ifdef __GNUC__
# if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5)
#  define EXITING
#  define NORETURN __attribute__ ((noreturn))
# else
#  define EXITING volatile	/* function never returns */
#  define NORETURN
# endif
#else
# define EXITING
# define NORETURN
#endif

/* define how to call functions with stack parameter passing */
#ifdef __TURBOC__
#define ARGS_ON_STACK cdecl
#else
#ifdef __CDECL
#define ARGS_ON_STACK __CDECL
#else
#define ARGS_ON_STACK
#endif
#endif

/* define to indicate unused variables */
#ifdef __TURBOC__
#define UNUSED(x)	(void)x
#else
#define UNUSED(x)
#endif

#ifndef P_
# ifdef __STDC__
#  define P_(x) x
# else
#  define P_(x) ()
# endif
#endif

#define ushort	unsigned short
typedef long (*Func)();

#define IOREC_T _IOREC

#include "filesys.h"

#define CTRL(x) ((x) & 0x1f)
#ifndef T_NOFLSH
#define T_NOFLSH	0x0040		/* don't flush buffer when signals
					   are received */
#endif

#if !RAW
#undef RAW
#undef ECHO
#define RAW T_RAW
#define ECHO T_ECHO
#define CRMOD T_CRMOD
#define CBREAK T_CBREAK
#define TOSTOP T_TOSTOP
#define XKEY T_XKEY
#endif

#ifndef READY_Q
#define CURPROC_Q	0
#define READY_Q		1
#define WAIT_Q		2
#define IO_Q		3
#define ZOMBIE_Q	4
#define TSR_Q		5
#define STOP_Q		6
#define SELECT_Q	7
#endif

extern struct kerinfo *kernel;

/* Fcntls for internal daemon/device communication, NOT for user processes...
   device checks caller's pid == daemon so they never should cause collisions.
*/
#define VCTLSETV	0x7fd0	/* show terminal (arg) */
#define VCTLFLASH	0x7fd1	/* flash current term's cursor */
#define VCTLWSEL	0x7fd2	/* wake select()ing readers on term (arg) */
