/*
hardware depandent physical screen and video mode stuff

virtual terminals vt01..vt09 are text-only so using big hi-res colour
screens for them (TT/falcon) only wastes memory and slows things down.
if possible use ST modes independent from whats on console...  also
to still have 80 columns when the console is running low res.

the problem here is there is no general OS call for this (display
another video mode without affecting GEMs) so this stuff is hardware
dependent.

thanx for the falcon parts goes to Georg Acher
<acher@informatik.tu-muenchen.de> and Frank Bartels
<knarf@nasim.sta.sub.org>; i can't test it but it works for them. :)

(more things i can't test are hardware additions like Overscan and
graphics cards, so if you get that working...)

compile with -DVTONEPLANE to use no colours on vt01..9 (faster)
*/

#include <stdio.h>
#include <mintbind.h>
#include "vcon.h"
#include "vtdev.h"

#ifdef FORCE1PLANE
#define VTONEPLANE
#endif

/* _VDO cookie values (video hardware) */
#define VDO_ST		0
#define VDO_STE		0x10000
#define VDO_TT		0x20000
#define VDO_FALCON	0x30000

static unsigned long vdo = 0;	/* _VDO cookie */
static int rez_vt = -1;		/* vt video mode (-1 == can't change) */
static short colour_white = 0xfff, colour_black = 0;
#if 0	/* not yet.. */
static short colour_1, colour_2;
#endif
static short ttcol_white = 0xfff, ttcol_black = 0;

static long f030col_white= 0xffffffff, f030col_black=0;


#define dbaseh (*(volatile char *)	0xffff8201)
#define vcounthi (*(volatile char *)	0xffff8205)
#define dbaselow (*(volatile char *)	0xffff820d)
#define linewidth (*(volatile char *)	0xffff820f)
#define color0 (*(short *)		0xffff8240)
#define color1 (*(short *)		0xffff8242)
#define color2 (*(short *)		0xffff8244)
#define color3 (*(short *)		0xffff8246)
#define shiftmd (*(unsigned char *)	0xffff8260)
#define shift_tt (*(unsigned short *)	0xffff8262)
#define hscroll (*(volatile char *)	0xffff8265)
#define tt_col ((short *)		0xffff8400)

/* for F030 */
#define f030_col ((long *)		0xffff9800)
#define f030_xreg ((short*) 		0xffff8282)
#define f030_yreg ((short*)		0xffff82a2)
#define f030_creg ((short*)		0xffff82c0)
#define f030_sreg ((short*)		0xffff8260)
#define f030_mreg ((short*)		0xffff820a)

#define tcdr (*(volatile char *)	0xfffffa23)

/* for F030 */
int nxreg[6]={0xc6,0x8d,0x15,0x273,0x50,0x96};
int nyreg[6]={0x419,0x3ff,0x3f,0x3f,0x3ff,0x415};
int nsreg[4]={0,0,0,0x400};
int ncreg[2]={0x186,0x08};
int nmreg[4]={0,0,0,0x28};


#ifdef __GNUC__
/* macro to turn interrupts off, result is the original sr. */
#define intsoff() \
({					\
	short retvalue;			\
	    				\
	__asm__ volatile		\
	(" movw    sr,%0;		\
	   orw     #0x700,sr; "		\
	: "=d"(retvalue)  /* outputs */	\
	);				\
	retvalue;			\
})

/* this turns them back on again, arg is the original sr */
#define intson(sr_) \
(void) ({				\
	short  _sr = (short) (sr_);	\
	    				\
	__asm__ volatile		\
	(" movw    %0,sr; "		\
	:		/* no output */	\
	: "d"(_sr)	/* inputs */	\
	);				\
})

/* funny way to do a movepw from C... */
#define readmovepw(add_) \
({					\
	char *add = (void *) (add_);	\
	short retvalue;			\
	    				\
	__asm__ volatile		\
	(" movepw    %1@(0),%0; "	\
	: "=d"(retvalue) /* outputs */	\
	: "ao"(add)	 /* inputs */	\
	);				\
	retvalue;			\
})

#define writemovepw(add_, word_) \
({					\
	char *add = (void *) (add_);	\
	short w = (short) (word_);	\
	    				\
	__asm__ volatile		\
	(" movepw    %1,%0@(0); "	\
	:		/* no output */	\
	: "ao"(add),"d"(w) /* inputs */	\
	);				\
})

#define lineA0fonts()				\
({	register char *retvalue __asm__("a1");	\
	__asm__ volatile("			\
	.word	0xa000 "			\
	: "=r"(retvalue)			\
	:					\
	: "d0", "d1", "d2", "a0", "a1", "a2"    \
	);					\
	retvalue;				\
})
#endif

/*
 * waiteoscreen(): wait for `end of screen', needed to safely write
 * some hardware registers... (STe shifter bug etc.)
 * returns with interrupts off, return value is original sr.
 */

INLINE static
short waiteoscreen()
{
	do {
		short vbas16, sr = intsoff ();
		char c;

		vbas16 = readmovepw (&dbaseh);
		if (vbas16 == readmovepw (&vcounthi)) {
			/* `below' end of screen already */
			intson (sr);
#if 0
			/* be nice to other processes...
			   not!  GEM is running in super mode too much :( */
			NAP (5);
#endif
			continue;
		}
		/* time out after 8 timer c ticks (@ 200*192 Hz), then allow
		   interrupts and try again to reduce receiver overruns etc. */
		c = tcdr;
		while (!(8 & (c - tcdr))) {
			if (vbas16 == readmovepw (&vcounthi))
				/* video counter just reset from end of screen
				   to beginning, this is the moment we want. */
				return sr;
		}
		intson (sr);
	} while (42);
}

/*
 * getvtmode (v00): find out the video mode to use for vt0[1-9], return
 * a SCREEN struct for it.  this gets called once at initialization
 * time before GEM is up (usually) i.e. it could mess with the consoles
 * video mode if it must.  v00 is console (vt00, readonly).
 * may use v0x[0], may allocate screen buffer itself (kcore, m_xalloc;
 * then adjust hardscroll and put pointer in returned struct).
 * note this is just initialisation, the real switching happens in
 * showscreen() below.
 */

SCREEN *getvtmode (v00)
	SCREEN *v00;
{
	long *cookie = *((long **) 0x5a0L);
	int rez_now, sysfont;
	short maxy;
	SCREEN *v;
	char ***cfonts;

	if (cookie) {
		while (*cookie) {
			if (*cookie == 0x5f56444fL) {	/* _VDO */
				vdo = cookie[1];
				break;
			}
			cookie += 2;
		}
	}
	switch (vdo) {
	case VDO_ST:
		/*FALLTHRU*/
	case VDO_STE:
		if ((rez_now = Getrez ()) < 2) {
			/* colour screen/tv, use st-med */
			colour_white = Setcolor (0, -1 );
			colour_black = Setcolor ((rez_now ? 3 : 15), -1);
			rez_vt = 1;
		} else
			/* st-hi */
	st_hi:		rez_vt = 2;
		maxy = 24;
	st_30:
		/* set up SCREEN struct for rez_vt (only entries we need) */
		bzero (v = v0x, sizeof (SCREEN));
		/* (this is really some struct fonthdr *[] but what the.. :) */
		cfonts = (char ***) lineA0fonts();

		v->maxx = 79;
		v->maxy = maxy;
		v->linelen = 1280;
		v->period = v00->period;
		v->form_width = 0x100;
		if (rez_vt == 1) {
			/* st-med */
			v->cheight = 8;
			v->fgcol = 3;
			v->planes = 2;
			v->planesiz = 160;
			sysfont = 1;
#ifdef VTONEPLANE
			v->fgcol = 1;
			v->v.t.usedplanes = 1;
#endif
		} else {
			/* st-hi */
			v->cheight = 16;
			v->fgcol = 1;
			v->planes = 1;
			v->planesiz = 80;
			sysfont = 2;
		}
		v->fontdata = cfonts[sysfont][19];
		return v;
	case VDO_TT:
		if ((rez_now = Getrez ()) == 6)
			/* tt-high, can't change mode */
			return v00;
		/* TeSche: Use black on white rather than vice versa*/
		if (rez_now < 2) {
			ttcol_black = Setcolor (0, -1);
			ttcol_white = Setcolor ((rez_now ? 3 : 15), -1);
		} else {
			ttcol_black = EsetColor ((rez_now == 2 ? 254 : 0), -1);
			ttcol_white = EsetColor ((rez_now == 4 ? 15 : 255), -1);
		}
		/* vga screen, use st-hi */
		goto st_hi;
	case VDO_FALCON:
		/* should this check for screen type?  */
		/* and could init f030col_white, f030col_black from whats on
		   console now (if people prefer white on black etc.)  */
		rez_vt = 2;
		maxy = 29; /* VGA 640*480*2 */
		goto st_30;
	default:
		/* dont know how to change video modes, use current one */
		return v00;
	}
}

/*
 * showscreen (vt, v, base, save): show console (vt == 0) or vt screen,
 * possibly changing video modes and save old one if we know how to.
 * this is also called for hardware scrolling so the case save==0 && vt!=0
 * should not use delays if possible. (-> just set address if that helps...)
 */

void showscreen (vt, v, vbase, save)
	int vt, save;
	SCREEN *v;
	char *vbase;
{
	static struct savedmode {
		unsigned mode;
		int	saved;
		short	vbase16;
		short	colours[4];
		char	vbaselow, vlinewidth, vhscroll;
		/* for Falcon030 */
		int xreg[6];
		int yreg[6];
		int creg[2];
		int sreg[4];
		int mreg[4];
		long f030colours[2];
	} console;
	struct savedmode *s = &console;
	short sr;

	if (rez_vt < 0) {
		/* unknown hardware or can't change mode -> only set address */
		Setscreen (-1l, vbase, -1l);
		return;
	}
	switch (vdo) {
	case VDO_ST:
		if (save) {
			s->vbase16 = readmovepw (&dbaseh);
			s->mode = shiftmd;
			s->colours[0] = color0;
			s->colours[1] = color1;
			s->colours[2] = color2;
			s->colours[3] = color3;
			s->saved = 1;
		}
		if (vt) {
			writemovepw (&dbaseh, ((unsigned long) vbase >> 8));
			if (save && s->mode != rez_vt) {
				sr = waiteoscreen ();
				shiftmd = rez_vt;
				color0 = colour_white;
				color3 = colour_black;
				intson (sr);
			}
#ifdef VTONEPLANE
			if (rez_vt < 2)
				color1 = colour_black;
#endif
		} else if (s->saved) {
			writemovepw (&dbaseh, s->vbase16);
			if (s->mode != rez_vt) {
				sr = waiteoscreen ();
				shiftmd = s->mode;
				color0 = s->colours[0];
				color1 = s->colours[1];
				color2 = s->colours[2];
				color3 = s->colours[3];
				intson (sr);
			}
#ifdef VTONEPLANE
			else if (rez_vt < 2)
				color1 = s->colours[1];
#endif
			s->saved = 0;
		}
		break;
	case VDO_STE:
		if (save) {
			s->vbase16 = readmovepw (&dbaseh);
			s->vbaselow = dbaselow;
			s->mode = shiftmd;
			s->vlinewidth = linewidth;
			s->vhscroll = hscroll;
			s->colours[0] = color0;
			s->colours[1] = color1;
			s->colours[2] = color2;
			s->colours[3] = color3;
			s->saved = 1;
		}
		if (vt) {
			if (save) {
				sr = waiteoscreen ();
				writemovepw (&dbaseh, ((unsigned long) vbase >> 8));
				dbaselow = (char) (long) vbase;
				linewidth = 0;
				hscroll = 0;
				if (s->mode != rez_vt) {
					shiftmd = rez_vt;
					color0 = colour_white;
					color3 = colour_black;
				}
#ifdef VTONEPLANE
				if (rez_vt < 2)
					color1 = colour_black;
#endif
			} else {
				sr = intsoff ();
				writemovepw (&dbaseh, ((unsigned long) vbase >> 8));
				dbaselow = (char) (long) vbase;
			}
			intson (sr);
		} else if (s->saved) {
			sr = waiteoscreen ();
			writemovepw (&dbaseh, s->vbase16);
			dbaselow = s->vbaselow;
			linewidth = s->vlinewidth;
			hscroll = s->vhscroll;
			shiftmd = s->mode;
			color0 = s->colours[0];
			color1 = s->colours[1];
			color2 = s->colours[2];
			color3 = s->colours[3];
			intson (sr);
			s->saved = 0;
		}
		break;
	case VDO_TT:
		if (save) {
			s->vbase16 = readmovepw (&dbaseh);
			s->vbaselow = dbaselow;
			s->mode = shift_tt;
			s->colours[0] = tt_col[0xfe];
			s->colours[1] = tt_col[0xff];
			s->saved = 1;
		}
		if (vt) {
			writemovepw (&dbaseh, ((unsigned long) vbase >> 8));
			dbaselow = (char) (long) vbase;
			if (save && s->mode != (rez_vt << 8)) {
				shift_tt = (rez_vt << 8);
				tt_col[0xfe] = ttcol_white;
				tt_col[0xff] = ttcol_black;
			}
		} else if (s->saved) {
			writemovepw (&dbaseh, s->vbase16);
			dbaselow = s->vbaselow;
			if (s->mode != rez_vt) {
				shift_tt = s->mode;
				tt_col[0xfe] = s->colours[0];
				tt_col[0xff] = s->colours[1];
			}
			s->saved = 0;
		}
		break;
	case VDO_FALCON:
		if (save)
			{
			int n;
			s->vbase16 = readmovepw (&dbaseh);
			s->vbaselow = dbaselow;
			
			for(n=0;n<6;n++)		/* Save F030 video stuff */
				{
				s->xreg[n]=f030_xreg[n];
				s->yreg[n]=f030_yreg[n];
				}
			s->creg[0]=f030_creg[0];
			s->creg[1]=f030_creg[1];
			s->sreg[0]=0;	/* preliminary */
			s->sreg[3]=f030_sreg[3];
			s->mreg[0]=f030_mreg[0];
			s->mreg[3]=f030_mreg[3];
			s->vlinewidth = linewidth;
			s->vhscroll = hscroll;
			s->f030colours[0] = f030_col[0x00];
			s->f030colours[1] = f030_col[0x01];
			s->saved = 1;
		}
		if (vt)
			{
			if (save)
				{
				int n;
				sr = waiteoscreen ();
				writemovepw (&dbaseh, ((unsigned long) vbase >> 8));
				dbaselow = (char) (long) vbase;
				linewidth = 0;
				hscroll = 0;

				f030_col[0]=f030col_white;
				f030_col[1]=f030col_black;

				for(n=0;n<6;n++)/* init F030 video stuff */
					{
					f030_xreg[n]=nxreg[n];
					f030_yreg[n]=nyreg[n];
					}
				f030_creg[0]=ncreg[0];
				f030_creg[1]=ncreg[1];
			/*	f030_sreg[0]=nsreg[0];*/	/* preliminary */
				f030_sreg[3]=nsreg[3];
				f030_mreg[0]=nmreg[0];
				f030_mreg[3]=nmreg[3];
			} else
				{
				sr=intsoff();
				writemovepw (&dbaseh, ((unsigned long) vbase >> 8));
				dbaselow = (char) (long) vbase;
			}
			intson(sr);
		} else		/* switch to saved mode */
			{
			int n;
			writemovepw (&dbaseh, s->vbase16);
			dbaselow = s->vbaselow;
			linewidth = s->vlinewidth;
			hscroll = s->vhscroll;
			/*sr=waiteoscreen();*/

			f030_col[0]=s->f030colours[0];
			f030_col[1]=s->f030colours[1];

			for(n=0;n<6;n++)		/* restore F030 video stuff */
				{
				f030_xreg[n]=s->xreg[n];
				f030_yreg[n]=s->yreg[n];
				}

			f030_creg[0]=s->creg[0];
			f030_creg[1]=s->creg[1];
			/*f030_sreg[0]=s->sreg[0];*/	/* preliminary */
			f030_sreg[3]=s->sreg[3];
			f030_mreg[0]=s->mreg[0];
			f030_mreg[3]=s->mreg[3];

			s->saved = 0;
			/*intson (sr);*/
		}
	}
}
