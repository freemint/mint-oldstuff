#include "filesys.h"

#ifdef __GNUC__
#define EXITING volatile	/* function never returns */
#else
#define EXITING
#endif

/* define how to call functions with stack parameter passing */
#ifdef __TURBOC__
#define ARGS_ON_STACK cdecl
#else
#define ARGS_ON_STACK
#endif

/* define to indicate unused variables */
#ifdef __TURBOC__
#define UNUSED(x)	(void)x
#else
#define UNUSED(x)
#endif

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

extern struct kerinfo *kernel;

/* Fcntls for internal daemon/device communication, NOT for user processes...
   device checks caller's pid == daemon so they never should cause collisions.
*/
#define VCTLSETV	0x7fd0	/* show terminal (arg) */
#define VCTLFLASH	0x7fd1	/* flash current term's cursor */
#define VCTLWSEL	0x7fd2	/* wake select()ing readers on term (arg) */
