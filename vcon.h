#if 1

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

#ifdef __STDC__
#define P_(x) x
#else
#define P_(x) ()
#define const
#define volatile
#endif

typedef unsigned short	ushort;
typedef long ARGS_ON_STACK (*Func)();
#include "file.h"
#else
#include "filesys.h"
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
#define CCONWS (void)(*kernel->dos_tab[0x09])

#define FOPEN (*kernel->dos_tab[0x3d])
#define FCLOSE (*kernel->dos_tab[0x3e])
#define FREAD (*kernel->dos_tab[0x3f])
#define MXALLOC (*kernel->dos_tab[0x44])
#define FDATIME (*kernel->dos_tab[0x44])
#define FCNTL (*kernel->dos_tab[0x104])
#define FINSTAT (*kernel->dos_tab[0x105])
#define FGETCHAR (*kernel->dos_tab[0x107])

#define SPRINTF (*kernel->sprintf)
#define DEBUG (*kernel->debug)
#define ALERT (*kernel->alert)
#define TRACE (*kernel->trace)
#define FATAL (*kernel->fatal)
#define KMALLOC (*kernel->kmalloc)
#define KFREE (*kernel->kfree)
#define SLEEP (*kernel->sleep)
#define WAKESELECT (*kernel->wakeselect)

