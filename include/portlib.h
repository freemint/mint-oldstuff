#ifndef _PORTLIB_H
#define _PORTLIB_H

#include <stdio.h>
#include <signal.h>
#include "sigvec.h"
#include "itimer.h"

#define _BSD_SOURCE

#ifndef TBDELAY
#define TBDELAY		0
#endif

#ifndef TIOCPKT
#define TIOCPKT_DATA		0
#define TIOCPKT_FLUSHREAD	1
#define TIOCPKT_FLUSHWRITE	2
#define TIOCPKT_STOP		4
#define TIOCPKT_START		8
#define TIOCPKT_NOSTOP		16
#define TIOCPKT_DOSTOP		32
#endif

#define MAXHOSTNAMELEN		64

#define L_SET			SEEK_SET
#define L_INCR			SEEK_CUR

#define d_namlen		d_reclen

#ifndef timerclear
#define timerclear(tm)	{ (tm)->tv_sec = (tm)->tv_usec = 0; }
#endif

typedef __Sigfunc sig_t;

__EXTERN int	__fork_and_exit (void);
__EXTERN int	__mint_write (int, void *, long);
__EXTERN int	__mint_write (int, void *, long);

__EXTERN int	initgroups (char *, int);

#endif
