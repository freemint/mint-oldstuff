#ifndef _SIGVEC_H
#define _SIGVEC_H

#include <signal.h>

struct sigvec {
	void 	(*sv_handler) (int);
	long	sv_mask;
	short	sv_flags;
#define sv_onstack	sv_flags;
};

#define SV_ONSTACK	(0x8000)
#define SV_INTERRUPT	(0x8000)
#define SV_RESETHAND	(0x8000)

__EXTERN int	sigvec __PROTO((int sig, struct sigvec *, struct sigvec *));
__EXTERN int	siginterrupt __PROTO((int sig, int fail));

#endif
