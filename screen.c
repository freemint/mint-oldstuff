/*
hardware depandent physical screen and video mode stuff
virtual terminals ttyv1..ttyv9 are text-only so using big hi-res colour
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

compile with -DVTONEPLANE to use no colours on ttyv1..9 (faster)

Steven Moore - made modifications for RGB monitor support.
               (works with SM124 monitor and TV as well.)
             - with this, comes colour support, and interlace/dline
               support. No dline or colour support for VGA, though... yet.

 "I had to make some fairly major patches. In particular, you'll need
  to make new /etc/resolution files using the NEW falconres.h and
  save_res.tos - the format's changed to include an st-comp flag.
  Oh, and it'll work with colour and/or interlaced/double line resolutions
  now (quite why you'd want to use doubleline or flickerlace, I've no idea,
  but there you go <g>)."  See README.Falcon for further information.
*/

#include <stdio.h>
#include <unistd.h>
#include <mintbind.h>
#include <falcon.h>
#include "vcon.h"
#include "vtdev.h"
/* for Falcon030 */
#include "falconres.h"

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

/*static long f030col_white= 0xffffffff, f030col_black=0;*/
static long f030col_white= 0, f030col_black= 0xffffffff; 
/* I like it, okay? Change them back over, if you like. -smoore 
   Hey, and for future reference: Never use white text on black background with
   SM124 mono monitor; when there are few bright pixels on an SM124, the
   brightness gets turned down... (why?)
   */

/* smoore: palette for 16 colour. (ANSI... I *think* <g>) */
static long f030_16_pal[16] = { 0x00000000, 0x88000000, 0x00880000, 0x88880000,
                                0x00000088, 0x88000088, 0x00880088, 0xb4b400b4,
                                0x55550055, 0xff000000, 0x00ff0000, 0xffff0000,
                                0x000000ff, 0xff0000ff, 0x00ff00ff, 0xffff00ff };

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

#define tcdr (*(volatile char *)	0xfffffa23)
	
/* Videl get/put routines.
**
** By Steven Moore, thanks to evil/dhs, chris/aura, 
** scandion/mugwumps, sage/escape. 
*/

/* VIDEO_putvideo : takes a VIDEL_DATA (VCF3) structure, 
   and writes it (correctly) to the Videl hardware.
   Translation: it changes resolutions. 

   First revision: smoore. 

   Caveat: Must be run in supervisor mode. 
*/
void VIDEO_putvideo(VIDEL_DATA *vd)
{
	int i;
	Vsync();
	VIDEL_hw_spshift[0]=0; /* Falcon shift clear */
	for(i=0; i<6; i++)
		VIDEL_hw_xregs[i]=vd->nxreg[i]; /* copy xregs */
	for(i=0; i<6; i++)
		VIDEL_hw_yregs[i]=vd->nyreg[i]; /* copy yregs */
	VIDEL_hw_vco[0]=vd->nvco; /* control flags */
	VIDEL_hw_c_s[0]=vd->nc_s;
	VIDEL_hw_offsets[0]=vd->noff[0]; /* line offset regs */
	VIDEL_hw_offsets[1]=vd->noff[1];
	VIDEL_hw_sync[0]=vd->nsync; /* sync reg */
	VIDEL_hw_p_o[0]=vd->np_o; /* horizontal planar offset */
	if (vd->st_flag) {
		VIDEL_hw_stshift[0]=vd->nsts; /* st compatible - write as follows */
		VIDEL_hw_c_s[0]=vd->nc_s; /* rewrite these three afterwards! */
		VIDEL_hw_offsets[0]=vd->noff[0];
		VIDEL_hw_offsets[1]=vd->noff[1];
	} else {
		Vsync(); /* necessary to avoid the famous sync-shifting-by-a-word bug */
		VIDEL_hw_spshift[0]=vd->nsps;
	}
}

/* VIDEO_getvideo : grabs the data on the
   current resolution, and saves it into a 
   VIDEL_DATA (VCF3) structure.

   First revision: smoore. 

   Caveat: Must be run in supervisor mode. 
*/
void VIDEO_getvideo(VIDEL_DATA *vd)
{
	int i;
	for(i=0; i<6; i++)
		vd->nxreg[i]=VIDEL_hw_xregs[i];
	for(i=0; i<6; i++)
  	vd->nyreg[i]=VIDEL_hw_yregs[i];
	vd->nvco=VIDEL_hw_vco[0];
	vd->nc_s=VIDEL_hw_c_s[0];
	vd->noff[0]=VIDEL_hw_offsets[0];
	vd->noff[1]=VIDEL_hw_offsets[1];
	vd->nsync=VIDEL_hw_sync[0];
	vd->np_o=VIDEL_hw_p_o[0];
	vd->st_flag=(VIDEL_hw_xregs[0] <= 0x00b0);  /* look out! logical expression */
	vd->nsps=VIDEL_hw_spshift[0];
	vd->nsts=VIDEL_hw_stshift[0];
}
/* END OF VIDEL DRIVER */

/* for Falcon030 */
VIDEL_DATA	videl_data;

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

void getvdocookie(void)
{
	long *cookie = *((long **) 0x5a0L);
	if (cookie) {
		while (*cookie) {
			if (*cookie == 0x5f56444fL) {	/* _VDO */
				vdo = cookie[1];
				break;
			}
			cookie += 2;
		}
	}
}

/* for Falcon030 */
void load_videl_data(void)
{
	int fd;
	VIDEL_DATA tmp_data;

	 VIDEL_DATA videl_data_vga640x480x2 = { VIDEL_DATA_IDENT, 80, 30,
			{ 0xc6, 0x8d, 0x15, 0x273, 0x50, 0x96 },
			{ 0x419, 0x3ff, 0x3f, 0x3f, 0x3ff, 0x415 },
			0x186, 0x08,
			{ 0x0, 0x28 },
			0x0, 0x0, 0x0, 0x400, 0x0 };

	VIDEL_DATA videl_data_rgb640x200x2 = { VIDEL_DATA_IDENT, 80, 25,
	 	{ 0x1ff, 0x197, 0x50, 0x3f0, 0x9f, 0x1b4 },
	  { 0x20d, 0x201, 0x16, 0x4d, 0x1dd, 0x207 },
	  0x185, 0x4, { 0x0, 0x28 }, 
	  0x0, 0x5, 0, 0x400, 0x100 };

/* Suitable default resolutions in case /etc/resolution
   doesn't exist yet... (saved by grab_res.tos)

   The routines *will* work with
   the SM124 - I've tried it - but it's a tempermental monitor and I didn't manage
   to get anything bigger than ST high, so it should never need to change res anyway;
   hence, no default res for SM124 needed. */

	Supexec(getvdocookie);

	if (vdo==VDO_FALCON) {
	  switch(Montype()) {
	  	case 3:   /* TV (same video capabilities as RGB, but usually we can't see as much border) */
				/* printf("none (TV)\r\n"); */
				videl_data = videl_data_rgb640x200x2;
				break;
	  	case 1:   /* RGB */
				/* printf("RGB (SC1224/1435)\r\n"); */
				videl_data = videl_data_rgb640x200x2;
				break;
	  	case 2:   /* VGA */
				/* printf("VGA/SVGA\r\n"); */
				videl_data = videl_data_vga640x480x2;
				break;
	  	case 0:   /* monochrome, it's okay; it can only do one resolution anyway ;) */
				/* printf("Monochrome (SM124/125)\r\n(Can't change resolution; only 640x400x2 supported!)\r\n"); */
				break;
      default:
        printf("unknown Montype()? (can't support)\r\n");
	  }
	}

	fd = open(VIDEL_DATA_FILENAME, O_RDONLY);
	if (fd < 0) {
#ifdef VIDEL_DATA_DEBUG
		printf("\r\nError %d : File "VIDEL_DATA_FILENAME" can't be opened\r\n", fd);
#endif
		return;
	}

	if (lseek(fd, 0L, SEEK_END) != sizeof(tmp_data)) {
		printf("\r\nFile "VIDEL_DATA_FILENAME" has unexpected size.\r\nRemember that 0.9b uses a different format than 0.9a!\r\n");
		return;
	}
	lseek(fd, 0L, SEEK_SET);
	read(fd, &tmp_data, sizeof(tmp_data));
	close(fd);

	if (strncmp(tmp_data.ident, VIDEL_DATA_IDENT, sizeof(tmp_data.ident))) {
		printf("\r\nIncorrect version of "VIDEL_DATA_FILENAME"\r\nRemember that 0.9b uses a different format than 0.9a!\r\n");
		return;
	}

	/* data file passed all security checks successfully */
	videl_data = tmp_data;

	/* printf("\r\nVirtual consoles use %d x %d resolution\r\n", videl_data.x_res, videl_data.y_res); */
}

/* the final trick for getting rid of the unfamous VIDEL bug with shifted screen */

/* Smoore's note: Man, that's some serious voodoo. Seems to work, though. 
   But the Videl routines above do it the right way anyway.. 
   The bug still happens sometimes, but how do we stop that? By running this 
   every X seconds? Might that make the screen flicker? Experimentation called
   for here. But, just changing to the console and back again will fix it :) */

/* INLINE void static VIDEL_resync(void)
{
	short old_w = *(short *)0xffff8266L;
	Vsync();
	*(short *)0xffff8266L = 0;
	Vsync();
	*(short *)0xffff8266L = old_w;
} */

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
 * getvtmode (v00): find out the video mode to use for ttyv[1-9], return
 * a SCREEN struct for it.  this gets called once at initialization
 * time before GEM is up (usually) i.e. it could mess with the consoles
 * video mode if it must.  v00 is console (ttyv0, readonly).
 * may use v0x[0], may allocate screen buffer itself (kcore, m_xalloc;
 * then adjust hardscroll and put pointer in returned struct).
 * note this is just initialisation, the real switching happens in
 * showscreen() below.
 */

SCREEN *getvtmode (v00)
	SCREEN *v00;
{
	long *cookie = *((long **) 0x5a0L);
	int rez_now, sysfont, sixteenhigh;
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
	st_hi:
		rez_vt = 2;
		maxy = 24;
	/* st_30: */
		/* set up SCREEN struct for rez_vt (only entries we need) */
		bzero (v = v0x, sizeof (SCREEN));
		/* (this is really some struct fonthdr *[] but what the.. :) */
		cfonts = (char ***) lineA0fonts();

		v->maxx = videl_data.x_res-1;
		v->maxy = maxy;
		v->linelen = (videl_data.x_res*16);
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
			v->planesiz = videl_data.x_res;
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
		/* get the preset resolution from configuration file */
		/* cannot open data file if called from here - so the call was
		   moved to main() of daemon.c */
		/* load_videl_data(); */

		switch (Montype ()) {
		/* WANTED:  video modes for the other screen types... */
    case 3: /* TV */
    case 1: /* RGB Monitor */
  		/* Well, now you've got 'em! - it's a hack, but it works. 
		   As soon as I can dig out the proper VIDEL docs, I'll be
		   coding a proper - UNIVERSAL - videl driver that'll work for
		   any resolution data (and therefore monitor) you throw at it. 
		   So there. -- smoore */
		/* I've found the docs. This is the current state of development. */

			/* set up SCREEN struct for rez_vt (only entries we need) */
			bzero (v = v0x, sizeof (SCREEN));
			/* How many planes?... technique works - usually. (Not for STlow) */
			if (videl_data.nsps & 0x400)
				v->planes = 1;
			else if (videl_data.nsps & 0x10)
				v->planes = 8;
			else if (videl_data.nsps & 0x100) { 
				printf("High colour resolutions are not yet implemented!\r\nVideo hardware support disabled.\r\nPlease correct "VIDEL_DATA_FILENAME".\r\n");
				return v00; /* Abort! Abort! */
			} else if (videl_data.st_flag)
				v->planes = 2;
			else { 	
				v->planes = 4; 
			}
			/* Select a sensible font - 16 pixels high, or 8? */
			sixteenhigh = 0;
			if (videl_data.nc_s & 2) sixteenhigh = 1;
			if (videl_data.nc_s & 1) sixteenhigh = 0;
			rez_vt = sixteenhigh ? 2 : 1; /* It's a bit like st-med or high, only... not really. */
			/* (this is really some struct fonthdr *[] but what the.. :) */
			cfonts = (char ***) lineA0fonts();
			v->maxx = videl_data.x_res-1;
			v->maxy = videl_data.y_res-1;
			v->cheight = sixteenhigh ? 16 : 8;          /* 16 for interlaced or vga, 8 for doubleline */
			sysfont = sixteenhigh ? 2 : 1;
			v->planesiz = ((videl_data.x_res)*(v->planes));
			v->linelen = ((videl_data.x_res)*(v->cheight)*(v->planes));
	 		v->period = v00->period;
			v->form_width = 0x100;
			v->width = v->linelen;
			v->fgcol = 1; v->fgcol <<= v->planes; v->fgcol--;
#ifdef VTONEPLANE
			v->fgcol = 1;
			v->v.t.usedplanes = 1;
#endif
			v->fontdata = cfonts[sysfont][19];
			return v;

    /* case 0: SM12(4|5) monochrome monitor - doesn't need explicit support */

		case 2: /* VGA colour monitor */
			bzero (v = v0x, sizeof (SCREEN));
			if (videl_data.nsps & 0x400)
				v->planes = 1;
			else if (videl_data.nsps & 0x10)
				v->planes = 8;
			else if (videl_data.nsps & 0x100) { 
				printf("High colour resolutions are not yet implemented!\r\nVideo hardware support disabled.\r\nPlease correct "VIDEL_DATA_FILENAME".\r\n");
				return v00;
			} else if (videl_data.st_flag)
				v->planes = 2;
			else { 	
				v->planes = 4; 
			}
			sixteenhigh = 1;
			if (videl_data.nc_s & 1) sixteenhigh = 0;
			rez_vt = sixteenhigh ? 2 : 1;
			cfonts = (char ***) lineA0fonts();
			v->maxx = videl_data.x_res-1;
			v->maxy = videl_data.y_res-1;
			v->cheight = sixteenhigh ? 16 : 8;
			sysfont = sixteenhigh ? 2 : 1;
			v->planesiz = ((videl_data.x_res)*(v->planes));
			v->linelen = ((videl_data.x_res)*(v->cheight)*(v->planes));
	 		v->period = v00->period;
			v->form_width = 0x100;
			v->width = v->linelen;
			v->fgcol = 1; v->fgcol <<= v->planes; v->fgcol--;
#ifdef VTONEPLANE
			v->fgcol = 1;
			v->v.t.usedplanes = 1;
#endif
			v->fontdata = cfonts[sysfont][19];
			return v;
			}
			/* FALLTHRU */

	default:
		/* dont know how to change modes without affecting GEM,
		   use current one */
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
		VIDEL_DATA videl;
		long f030colours[16];
	} console;
	struct savedmode *s = &console;
	short sr;

	if (rez_vt < 0) {
		/* unknown hardware or can't change mode -> only set address */
		(void) Setscreen (-1l, vbase, -1, -1);
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
			if (s->mode != rez_vt) {
				shiftmd = s->mode;
				color0 = s->colours[0];
				color1 = s->colours[1];
				color2 = s->colours[2];
				color3 = s->colours[3];
			}
#ifdef VTONEPLANE
			else if (rez_vt < 2)
				color1 = s->colours[1];
#endif
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
			
			VIDEO_getvideo(&(s->videl));

			for(n=0; n<16; n++) s->f030colours[n] = f030_col[n];

			s->saved = 1;
		}
		if (vt)
		{
			if (save)
			{
				int n;
				sr = waiteoscreen ();
				Vsync();

				writemovepw (&dbaseh, ((unsigned long) vbase >> 8));
				dbaselow = (char) (long) vbase;
/*				linewidth = 0;
				hscroll = 0;*/

#ifndef FORCE1PLANE
				if (v->planes == 4)
					for (n=0; n<16; n++) f030_col[n]=f030_16_pal[n];
				/* else if (v->planes == 2) {

					 Some ST-type palette support should go here,
                                           but isn't implemented.

					}*/
				else {
#endif
					f030_col[0]=f030col_white;
					f030_col[1]=f030col_black;
#ifndef FORCE1PLANE
				}
#endif

				VIDEO_putvideo(&videl_data);

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
/*			linewidth = s->vlinewidth;
			hscroll = s->vhscroll;*/
			sr=waiteoscreen();
			Vsync();

			for(n=0; n<16; n++) f030_col[n] = s->f030colours[n];

			VIDEO_putvideo(&(s->videl));

			s->saved = 0;

			intson (sr);
		}
	}
}
