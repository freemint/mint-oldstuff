/*
virtual terminal devices, based on MiNTs fasttext.c, that is...

Copyright 1991,1992 Eric R. Smith.
Copyright 1992,1993 Atari Corporation.
All rights reserved.
*/

#include <stddef.h>
#include <errno.h>
#include <osbind.h>
#include "vcon.h"
#include "vtdev.h"

#ifdef __GNUC__
#define INLINE inline
#define ITYPE long	/* gcc's optimizer likes 32 bit integers */
#else
#define INLINE
#define ITYPE int
#endif

#define CONDEV	(2)

SCREEN *v00, v0x[N_VT-1];

static void paint P_((SCREEN *, int, char *)),
	 paint8c P_((SCREEN *, int, char *)),
	 paint816m P_((SCREEN *, int, char *));

INLINE static void curs_off P_((SCREEN *)), curs_on P_((SCREEN *));
INLINE static void flash P_((SCREEN *));
static void normal_putch P_((SCREEN *, int));
static void escy_putch P_((SCREEN *, int));
static void quote_putch P_((SCREEN *, int));

static	char *chartab[256];

static int fgmask[MAX_PLANES], bgmask[MAX_PLANES];

static long scrnsize;

short hardscroll;
static char *hardbase, *oldbase;

#define base (*((char **)0x44eL))
#define V_BASE(v) ((v) == v00 ? base : (v)->v.t.vbase)
#define V_LINE(v, lx4) ((v) == v00 ? (base + *(long *)(rowoff+(lx4))) : \
			((v)->v.t.vbase + *(long *)((v)->v.t.rowlist+(lx4))))
#define V_LINEAR_P(v) ((v) == v00 || (v)->v.t.on)
#define VT_SCREEN(vt) ((vt) ? v0x+(vt)-1 : v00)
#define escy1 (*((short *)0x4acL))
#define V_ESCY1(v) ((v) == v00 ? escy1 : (v)->v.t.vescy1)
#define V_FGMASK(v) ((v) == v00 ? fgmask : (v)->v.t.fgmask)
#define V_BGMASK(v) ((v) == v00 ? bgmask : (v)->v.t.bgmask)
#define _hz_200 (*((long *)0x4baL))

static Vfunc v00state;
#define V_STATE(v) ((v) == v00 ? &v00state : &(v)->v.t.state)

static short hardline;
static void (*vpaint) P_((SCREEN *, int, char *));
static char *rowoff;
static short qfd[N_VT], q_fl[N_VT];

void exchangeb P_((void *, void *, long));
void init P_((void));
int setcurrent P_((int));
void hardware_scroll P_((SCREEN *));
INLINE static char *PLACE P_((SCREEN *, int, int));
INLINE static void gotoxy P_((SCREEN *, int, int));
INLINE static void clrline P_((SCREEN *, int));
INLINE static void clear P_((SCREEN *));
INLINE static void clrchars P_((SCREEN *, int, int, int));
INLINE static void clrfrom P_((SCREEN *, int, int, int, int));
INLINE static void delete_line P_((SCREEN *, int));
INLINE static void insert_line P_((SCREEN *, int));
static void setbgcol P_((SCREEN *, int));
static void setfgcol P_((SCREEN *, int));
static void setcurs P_((SCREEN *, int));
static void putesc P_((SCREEN *, int));
static void escy1_putch P_((SCREEN *, int));
#if 0
INLINE static void put_ch P_((SCREEN *, int));
#else
INLINE static void put_ch00 P_((SCREEN *, int));
INLINE static void put_ch0x P_((SCREEN *, int));
#endif

/* routines for flashing the cursor for screen v */
/* flash(v): invert the character currently under the cursor */

INLINE static void
flash(v)
	SCREEN *v;
{
	char *place;
	ITYPE i, j, vplanes;

	vplanes = v->planes + v->planes;
	place = v->cursaddr;

	for (j = v->cheight; j > 0; --j) {
		for (i = 0; i < vplanes; i+=2)
			place[i] = ~place[i];

		place += v->planesiz;
	}
	v->curstimer = v->period;
}

/* actually flash cursor (called from vcon.c) */

void xflash()
{
	SCREEN *v = v0x+vcurrent-1;

	/* vt00's cursor is handled by TOS... */
	if (!vcurrent || v->hidecnt)
		return;
	if ((CURS_FLASH|CURS_ON) == (v->flags & (CURS_FLASH|CURS_ON))) {
		flash(v);
		v->flags ^= CURS_FSTATE;
	}
}

/* make sure the cursor is off */

INLINE
static void
curs_off(v)
	SCREEN *v;
{
	if (v->flags & CURS_ON) {
		if (v->flags & CURS_FSTATE) {
			flash(v);
			v->flags &= ~CURS_FSTATE;
		}
	}
}

/* OK, show the cursor again (if appropriate) */

INLINE static void
curs_on(v)
	SCREEN *v;
{
	if (v->hidecnt) return;

	if (v->flags & CURS_ON) {
#if 0
	/* if the cursor is flashing, we cheat a little and leave it off
	 * to be turned on again (if necessary) by the VBL routine
	 */
		if (v->flags & CURS_FLASH) {
			v->curstimer = 2;
			return;
		}
#endif
		if (!(v->flags & CURS_FSTATE)) {
			/* if you can't see the cursor there's no
			   reason to flash it */
			if ((v->flags & CURS_FLASH) &&
			    v != v00 && (!vcurrent || !v->v.t.on))
				return;
			v->flags |= CURS_FSTATE;
			flash(v);
		}
	}
}

#ifdef __GNUC__
#define lineA0()				\
({	register char *retvalue __asm__("d0");	\
	__asm__ volatile("			\
	.word	0xa000 "			\
	: "=r"(retvalue)			\
	:					\
	: "d0", "d1", "d2", "a0", "a1", "a2"    \
	);					\
	retvalue;				\
})
#endif

/* init vt0[1-9] SCREEN struct */

void
init_screen(v, vbase, rowlist, on)
	SCREEN *v;
	char *vbase, *rowlist;
	short on;
{
	static char initv00[sizeof (SCREEN) - offsetof (SCREEN, cheight)];

	if (on)
		memmove (initv00, (char *)&v00->cheight, sizeof (initv00));
	bzero ((char *)v, offsetof (SCREEN, cheight));
	memmove ((char *)&v->cheight, initv00, sizeof (initv00));

	v->v.t.vbase = vbase;
	v->v.t.rowlist = rowlist;
	v->v.t.on = on;
	v->v.t.state = normal_putch;
	v->cursaddr = vbase;
	v->cx = 0; v->cy = 0;
	v->flags = CURS_ON|CURS_FLASH|FWRAP;
	setbgcol(v, v->bgcol);
	setfgcol(v, v->fgcol);
	clear(v);
}

void
init()
{
	SCREEN *v;
	int i, j;
	char *data, *foo;
	static char chardata[256*16];
	register int linelen;

	foo = lineA0();
	v = v00 = (SCREEN *)(foo - 346);
	
	/* Ehem... The screen might be bigger than 32767 bytes.
	   Let's do some casting... 
	   Erling
	*/
	linelen = v->linelen;
	scrnsize = (v->maxy+1)*(long)linelen;
	rowoff = (char *)kmalloc((long)((v->maxy+1) * sizeof(long) * (N_VT-1)));
	if (rowoff == 0) {
		FATAL("Insufficient memory for screen offset table!");
	} else {
		long off, *lptr = (long *)rowoff;
		SCREEN *vp = v0x+1;

		for (i=0, off=0; i<=v->maxy; i++) {
			*lptr++ = off;
			off += linelen;
		}
		for (i=0; i<N_VT-1; i++) {
			(vp++)->v.t.rowlist = (char *)lptr;
			lptr += v->maxy+1;
		}
	}
	if (hardscroll == -1) {
	/* request for auto-setting */
		hardscroll = v->maxy+1;
	}
	if (!hardbase) {
		hardbase = (char *)(((long)kcore(SCNSIZE(v)+256L)+255L)
					   & 0xffffff00L);
		if (hardbase == 0)
			FATAL("Insufficient memory for second screen buffer!");
		init_screen(v0x, hardbase, rowoff, V_FREE);
	}
	hardline = 0;
	if (v->cheight == 8 && v->planes == 2) {
		foo = &chardata[0];
		vpaint = paint8c;
		for (i = 0; i < 256; i++) {
			chartab[i] = foo;
			data = v->fontdata + i;
			for (j = 0; j < 8; j++) {
				*foo++ = *data;
				data += v->form_width;
			}
		}
	} else if ((v->cheight == 16 || v->cheight == 8) && v->planes == 1) {
		foo = &chardata[0];
		vpaint = paint816m;
		for (i = 0; i < 256; i++) {
			chartab[i] = foo;
			data = v->fontdata + i;
			for (j = 0; j < v->cheight; j++) {
				*foo++ = *data;
				data += v->form_width;
			}
		}
	}
	else
		vpaint = paint;

	if (v->hidecnt == 0) {
	/*
	 * make sure the cursor is set up correctly and turned on
	 */
		(void)Cursconf(0,0);	/* turn cursor off */

		v->flags &= ~CURS_FSTATE;

	/* now turn the cursor on the way we like it */
		v->curstimer = v->period;
		v->hidecnt = 0;
		v->flags |= CURS_ON;
		curs_on(v);
	} else {
		(void)Cursconf(0,0);
		v->flags &= ~CURS_ON;
		v->hidecnt = 1;
	}

	/* setup bgmask and fgmask */
	setbgcol(v, v->bgcol);
	setfgcol(v, v->fgcol);
	*V_STATE(v) = normal_putch;
}

/* deinit, must be called after last close */

void
deinit()
{
	kfree (rowoff);
}

/* exchange memory, assumes pointers word-aligned and bytes
   multiple of sizeof long...  (faster implementations welcome :-)
*/

INLINE
void
exchangeb(x1, x2, bytes)
	void *x1, *x2;
	long bytes;
{
	long *p, *q, t;

	for (p = x1, q = x2; bytes > 0; bytes -= sizeof (long)) {
		t = *p;
		*p++ = *q;
		*q++ = t;
	}
}

/*
 * PLACE(v, x, y): the address corresponding to the upper left hand corner of
 * the character at position (x,y) on screen v
 */
INLINE static
char *PLACE(v, x, y)
	SCREEN *v;
	int x, y;
{
	char *place;
	int i, j;

	if (V_LINEAR_P(v)) {
		place = V_BASE(v) + x;
		if (y == v->maxy)
			place += scrnsize - v->linelen;
		else if (y) {
			y+=y;	/* Make Y into index for longword array. */
			y+=y;	/* Two word-size adds are faster than a 2-bit shift. */
			place += *(long *)(rowoff + y);
		}
	} else {
		y+=y;	/* Make Y into index for longword array. */
		y+=y;	/* Two word-size adds are faster than a 2-bit shift. */
		place = V_LINE(v, y) + x;
	}
	if ((j = v->planes-1)) {
		i = (x & 0xfffe);
		do place += i;
		while (--j);
	}
	return place;
}

int
setcurrent(vt)
	int vt;
{
	static int v0xcurrent = 1;
	SCREEN *v = VT_SCREEN(vt);

	/* are we changing to a `stored' screen? */
	if (vt && vt != v0xcurrent) {
		SCREEN *oldv = VT_SCREEN(v0xcurrent);
		char *foo, *vline = oldv->v.t.vbase;
		int i;

		if (!v->v.t.vbase)
			/* sorry terminal closed, has no screen memory */
			return 1;

		/* exchange screen contents... */
		for (i=0; i<=v->maxy*sizeof (long); i+=sizeof (long)) {
			exchangeb (vline, V_LINE(v, i), v->linelen);
			vline += v->linelen;
		}
		/* and pointers... */
		foo = oldv->v.t.vbase;
		oldv->v.t.vbase = v->v.t.vbase;
		v->v.t.vbase = foo;
		foo = oldv->v.t.rowlist;
		oldv->v.t.rowlist = v->v.t.rowlist;
		v->v.t.rowlist = foo;

		/* free screen memory if told so */
		if (oldv->v.t.on == V_FREE) {
			oldv->v.t.on = 0;
			kfree (oldv->v.t.vbase);
			oldv->v.t.vbase = 0;
		} else {
			oldv->v.t.on = 0;
			oldv->cursaddr = PLACE(oldv, oldv->cx, oldv->cy);
		}
		v->v.t.on = V_USED;
		v->cursaddr = PLACE(v, v->cx, v->cy);
		v0xcurrent = vt;
	}
	vcurrent = vt;
	if (vt && (v->flags & CURS_FLASH))
		curs_on(v);
	Setscreen(-1l, V_BASE(v), -1);
	return 0;
}

/*
 * paint(v, c, place): put character 'c' at position 'place' on screen
 * v. It is assumed that x, y are proper coordinates!
 * Specialized versions (paint8c and paint816m) of this routine follow;
 * they assume 8 line high characters, medium res. and 8 or 16 line/mono,
 * respectively.
 */

static void
paint(v, c, place)
	SCREEN *v;
	int c;
	char *place;
{
	char *data, d, doinverse;
	ITYPE j, planecount;
	int vplanes;
	long vform_width, vplanesiz;
	int *fgmaskv = V_FGMASK(v), *bgmaskv = V_BGMASK(v);

	vplanes = v->planes;

	data = v->fontdata + c;
	doinverse = (v->flags & FINVERSE) ? 0xff : 0;
	vform_width = v->form_width;
	vplanesiz = v->planesiz;

	for (j = v->cheight-1; j > 0; --j) {
		d = *data ^ doinverse;
		for (planecount = 0; planecount < vplanes; planecount++)
		  place[planecount << 1]
		    = ((d & (char) fgmaskv[planecount])
		       | (~d & (char) bgmaskv[planecount]));
		place += vplanesiz;
		data += vform_width;
	}
	d = ((v->flags & FUNDERLINE) ? -1 : *data) ^ doinverse;
	for (planecount = 0; planecount < vplanes; planecount++)
	  place[planecount << 1]
	    = ((d & (char) fgmaskv[planecount])
	       | (~d & (char) bgmaskv[planecount]));
}

static void
paint8c(v, c, place)
	SCREEN *v;
	int c;
	char *place;
{
	char *data;
	char d, doinverse, dounderline;
	char bg0, bg1, fg0, fg1;
	long vplanesiz;
	int *m;

	data = chartab[c];

	doinverse = (v->flags & FINVERSE) ? 0xff : 0;
	dounderline = (v->flags & FUNDERLINE) ? 0xff : 0;
	vplanesiz = v->planesiz;
	m = V_BGMASK(v);
	bg0 = *m++;
	bg1 = *m++;
	m = V_FGMASK(v);
	fg0 = *m++;
	fg1 = *m++;

	if (!doinverse && !bg0 && !bg1 && fg0 && fg1) {
		/* line 1 */
		d = *data++;
		*place = d;
		place[2] = d;
		place += vplanesiz;

		/* line 2 */
		d = *data++;
		*place = d;
		place[2] = d;
		place += vplanesiz;

		/* line 3 */
		d = *data++;
		*place = d;
		place[2] = d;
		place += vplanesiz;

		/* line 4 */
		d = *data++;
		*place = d;
		place[2] = d;
		place += vplanesiz;

		/* line 5 */
		d = *data++;
		*place = d;
		place[2] = d;
		place += vplanesiz;

		/* line 6 */
		d = *data++;
		*place = d;
		place[2] = d;
		place += vplanesiz;

		/* line 7 */
		d = *data++;
		*place = d;
		place[2] = d;
		place += vplanesiz;

		/* line 8 */
		d = *data | dounderline;
		*place = d;
		place[2] = d;
	} else {
		/* line 1 */
		d = *data++ ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
		place += vplanesiz;

		/* line 2 */
		d = *data++ ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
		place += vplanesiz;

		/* line 3 */
		d = *data++ ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
		place += vplanesiz;

		/* line 4 */
		d = *data++ ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
		place += vplanesiz;

		/* line 5 */
		d = *data++ ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
		place += vplanesiz;

		/* line 6 */
		d = *data++ ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
		place += vplanesiz;

		/* line 7 */
		d = *data++ ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
		place += vplanesiz;

		/* line 8 */
		d = (*data | dounderline) ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
	}
}

static void
paint816m(v, c, place)
	SCREEN *v;
	int c;
	char *place;
{
	char *data;
	char d, doinverse, dounderline;
	long vplanesiz;

	data = chartab[c];
	doinverse = (v->flags & FINVERSE) ? 0xff : 0;
	doinverse ^= (d = V_BGMASK(v)[0]);
	dounderline = (v->flags & FUNDERLINE) ? 0xff : 0;
	vplanesiz = v->planesiz;

	if (d == V_FGMASK(v)[0])
	  {
	    /* fgcol and bgcol are the same -- easy */
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    if (v->cheight == 8)
		return;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	  }
	else if (!doinverse) {
		/* line 1 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 2 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 3 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 4 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 5 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 6 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 7 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 8 */
		d = *data++;
		if (v->cheight == 8) {
			*place = d | dounderline;
			return;
		}
		*place = d;

		place += vplanesiz;

		/* line 9 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 10 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 11 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 12 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 13 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 14 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 15 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 16 */
		d = *data;
		*place = d | dounderline;
	} else {
		/* line 1 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 2 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 3 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 4 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 5 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 6 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 7 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 8 */
		d = ~*data++;
		if (v->cheight == 8) {
			*place = d | dounderline;
			return;
		}
		*place = d;

		place += vplanesiz;

		/* line 9 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 10 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 11 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 12 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 13 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 14 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 15 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 16 */
		d = ~*data;
		*place = d | dounderline;
	}
}

/*
 * gotoxy (v, x, y): move current cursor address of screen v to (x, y)
 * makes sure that (x, y) will be legal
 */

INLINE static void
gotoxy(v, x, y)
	SCREEN *v;
	int x, y;
{
	if (x > v->maxx) x = v->maxx;
	else if (x < 0) x = 0;
	if (y > v->maxy) y = v->maxy;
	else if (y < 0) y = 0;

	v->cx = x;
	v->cy = y;
	v->cursaddr = PLACE(v, x, y);
}

/*
 * clrline(v, r): clear line r of screen v
 */

INLINE static void
clrline(v, r)
	SCREEN *v;
	int r;
{
	int *dst, *m;
	long nwords;
	int i, vplanes = v->planes;

	/* Hey, again the screen might be bigger than 32767 bytes.
	   Do another cast... */
	r += r;
	r += r;
	dst = (int *)(V_LINE(v, r));
	if (v->bgcol == 0)
	  zero((char *)dst, v->linelen);
	else if (vplanes == 1)
	  memset ((char *)dst, *V_BGMASK(v), v->linelen);
	else
	  {
	    /* do it the hard way */
	    for (nwords = v->linelen >> 1; nwords > 0; nwords -= vplanes)
	      {
		m = V_BGMASK(v);
		for (i = 0; i < vplanes; i++)
		  *dst++ = *m++;
	      }
	  }
}
	
/*
 * clear(v): clear the whole screen v
 */

INLINE static void
clear(v)
	SCREEN *v;
{
	int i, vplanes = v->planes;
	int *dst = (int *) V_BASE(v), *m;
	long nwords;

	if (!V_LINEAR_P(v))
	  memmove (v->v.t.rowlist, rowoff, ((v->maxy+1) * sizeof(long)));
	if (v->bgcol == 0)
	  zero((char *)dst, scrnsize);
	else if (vplanes == 1)
	  memset ((char *)dst, *V_BGMASK(v), scrnsize);
	else
	  {
	    /* do it the hard way */
	    for (nwords = scrnsize >> 1; nwords > 0; nwords -= vplanes)
	      {
		m = V_BGMASK(v);
		for (i = 0; i < vplanes; i++)
		  *dst++ = *m++;
	      }
	  }
}

/*
 * clrchars(v, x, y, n): clear n chars starting at position (x,y) on screen v
 */

/*INLINE*/ static void
clrchars(v, x, y, n)
	SCREEN *v;
	int x, y, n;
{
	int i, j, vplanes;
	char *place;
	int *m, *l;

	if (!x && n == v->maxx+1) {
		clrline(v, y);
		return;
	}
	vplanes = v->planes + v->planes;

	if (y == v->cy && x == v->cx)
		place = v->cursaddr;
	else
		place = PLACE(v, x, y);

	l = V_BGMASK(v);
	if (vplanes > 2) {
		if (x & 1) {
			char *p = place;
			for (j = v->cheight; j > 0; --j) {
				char *q = p;
				m = l;
				for (i = 0; i < vplanes; i += 2) {
					*q++ = (char) *m++;
					++q;
				}
				p += v->planesiz;
			}
			place += vplanes-1;
			--n;
		}
		if (n > 1) {
			int nbytes = n*(vplanes>>1);
			char *p = place;
			place += nbytes;

			if (v->bgcol == 0) {
				for (j = v->cheight; j > 0; --j) {
					bzero(p, nbytes);
					p += v->planesiz;
				}
			} else {
				for (j = v->cheight; j > 0; --j) {
					short *q = (short *)p;
					int k;

					for (k = n; k > 1; k -= 2) {
						m = l;
						for (i = 0; i < vplanes; i += 2)
							*q++ = *m++;
					}
					p += v->planesiz;
				}
			}
		}
		if (n & 1) {
			for (j = v->cheight; j > 0; --j) {
				char *p = place;
				m = l;
				for (i = 0; i < vplanes; i += 2) {
					*p++ = (char) *m++;
					++p;
				}
			}
			place += v->planesiz;
		}
	} else {
		for (j = v->cheight; j > 0; --j) {
			memset (place, *l, n);
			place += v->planesiz;
		}
	}
}

/*
 * clrfrom(v, x1, y1, x2, y2): clear screen v from position (x1,y1) to
 * position (x2, y2) inclusive. It is assumed that y2 >= y1.
 */

INLINE static void
clrfrom(v, x1, y1, x2, y2)
	SCREEN *v;
	int x1,y1,x2,y2;
{
	int i;

	clrchars(v, x1, y1, (y2 == y1 ? x2 : v->maxx)-x1+1);
	if (y2 > y1) {
		for (i = y1+1; i < y2; i++)
			clrline(v, i);
		clrchars(v, 0, y2, x2);
	}
}

/*
 * scroll a screen in hardware; if we still have hardware scrolling lines left,
 * just move the physical screen base, otherwise copy the screen back to the
 * hardware base and start over
 */
void
hardware_scroll(v)
	SCREEN *v;
{

	++hardline;
	if (hardline < hardscroll) { /* just move the screen */
		v->v.t.vbase += v->linelen;
	} else {
		hardline = 0;
		quickmove(hardbase, v->v.t.vbase + v->linelen, scrnsize - v->linelen);
		v->v.t.vbase = hardbase;
	}
	v->cursaddr = PLACE(v, v->cx, v->cy);
	if (vcurrent)
		Setscreen(-1l, v->v.t.vbase, -1);
}

/*
 * delete_line(v, r): delete line r of screen v. The screen below this
 * line is scrolled up, and the bottom line is cleared.
 */

#define scroll(v) delete_line(v, 0)

INLINE static void
delete_line(v, r)
	SCREEN *v;
	int r;
{
	long *src, *dst, nbytes;

	/* if this screen needs not be linear (i.e. its `stored' not shown)
	   then just adjust the line offset table...
	*/
	if (!V_LINEAR_P(v)) {
		register int i = r + r;
		long t;
		i += i;
		src = (long *)(v->v.t.rowlist+i);
		i = v->maxy - r;
		i += i;
		i += i;
		t = *src;
		memmove (src, src+1, i);
		i = v->maxy + v->maxy;
		i += i;
		*(long *)(v->v.t.rowlist+i) = t;
		clrline(v, v->maxy);
		return;
	}
	if (r == 0) {
		if (v != v00 & hardscroll > 0) {
			hardware_scroll(v);
			clrline(v, v->maxy);
			return;
		}
		nbytes = scrnsize - v->linelen;
	} else {
		register int i = v->maxy - r;
		i += i;
		i += i;
		nbytes = *(long *)(rowoff+i);
	}

	/* Sheeze, how many times do we really have to cast... 
	   Erling.	
	*/

	r += r;
	r += r;
	dst = (long *)(V_BASE(v) + *(long *)(rowoff + r));
	src = (long *)( ((long)dst) + v->linelen);

	quickmove(dst, src, nbytes);

/* clear the last line */
	clrline(v, v->maxy);
}

void
hardware_scroll_down(v)
	SCREEN *v;
{

	--hardline;
	if (hardline >= 0) { /* just move the screen */
		v->v.t.vbase -= v->linelen;
	} else {
		hardline = hardscroll - 1;
		v->v.t.vbase = hardbase + (long) hardline*v->linelen;
		memmove(v->v.t.vbase + v->linelen, hardbase, scrnsize - v->linelen);
	}
	v->cursaddr = PLACE(v, v->cx, v->cy);
	if (vcurrent)
		Setscreen(-1l, v->v.t.vbase, -1);
}

/*
 * insert_line(v, r): scroll all of the screen starting at line r down,
 * and then clear line r.
 */

INLINE static void
insert_line(v, r)
	SCREEN *v;
	int r;
{
	long *src, *dst;
	int i, j, linelen;

	if (!V_LINEAR_P(v)) {
		long t;
		i = r + r;
		i += i;
		src = (long *)(v->v.t.rowlist+i);
		i = v->maxy + v->maxy;
		i += i;
		t = *(long *)(v->v.t.rowlist+i);
		i = v->maxy - r;
		i += i;
		i += i;
		memmove (src+1, src, i);
		*src = t;
		clrline(v, r);
		return;
	}
	if (!r && v != v00 & hardscroll > 0) {
		hardware_scroll_down(v);
		clrline(v, 0);
		return;
	}
	i = v->maxy - 1;
	i += i;
	i += i;
	j = r+r;
	j += j;
	linelen = v->linelen;
	src = (long *)(V_BASE(v) + *(long *)(rowoff + i));
	dst = (long *)((long)src + linelen);
	for (; i >= j ; i -= 4) {
	/* move line i to line i+1 */
		quickmove(dst, src, linelen);
		dst = src;
		src = (long *)((long) src - linelen);
	}

/* clear line r */
	clrline(v, r);
}

/*
 * special states for handling ESC b x and ESC c x. Note that for now,
 * color is ignored.
 */

static void
setbgcol(v, c)
	SCREEN *v;
	int c;
{
	int i;
	int *m = V_BGMASK(v);

	v->bgcol = c & ((1 << v->planes)-1);
	for (i = 0; i < v->planes; i++)
	    *m++ = (v->bgcol & (1 << i)) ? -1 : 0;
	*V_STATE(v) = normal_putch;
}

static void
setfgcol(v, c)
	SCREEN *v;
	int c;
{
	int i;
	int *m = V_FGMASK(v);

	v->fgcol = c & ((1 << v->planes)-1);
	for (i = 0; i < v->planes; i++)
	    *m++ = (v->fgcol & (1 << i)) ? -1 : 0;
	*V_STATE(v) = normal_putch;
}

static void
setcurs(v, c)
	SCREEN *v;
	int c;
{
	c -= ' ';
	if (!c) {
		v->flags &= ~CURS_FLASH;
	} else {
		v->flags |= CURS_FLASH;
		v->period = (unsigned char) c;
	}
	*V_STATE(v) = normal_putch;
}

/* set special effects...  FIXME: only inverse and underline do anything */
static void
seffect_putch(v, c)
	SCREEN *v;
	int c;
{
	v->flags |= ((c & 0x10) ? FINVERSE : 0)|((c & 0x8) ? FUNDERLINE : 0);
	*V_STATE(v) = normal_putch;
}

/* clear special effects */
static void
ceffect_putch(v, c)
	SCREEN *v;
	int c;
{
	v->flags &= ~(((c & 0x10) ? FINVERSE : 0)|((c & 0x8) ? FUNDERLINE : 0));
	*V_STATE(v) = normal_putch;
}

static void
quote_putch(v, c)
	SCREEN *v;
	int c;
{
	(*vpaint)(v, c, v->cursaddr);
	*V_STATE(v) = normal_putch;
}

/*
 * putesc(v, c): handle the control sequence ESC c
 */

static void
putesc(v, c)
	SCREEN *v;
	int c;
{
	int i;
	int cx, cy;

	cx = v->cx; cy = v->cy;

	switch (c) {
	case 'A':		/* cursor up */
		if (cy) {
moveup:			v->cy = --cy;
			if (V_LINEAR_P(v))
				v->cursaddr -= v->linelen;
			else {
				long *r;
				i = cy + cy;
				i += i;
				r = (long *)(v->v.t.rowlist+i);
				v->cursaddr -= r[1] - r[0];
			}
		}
		break;
	case 'B':		/* cursor down */
		if (cy < v->maxy) {
			v->cy = ++cy;
			if (V_LINEAR_P(v))
				v->cursaddr += v->linelen;
			else {
				long *r;
				i = cy + cy;
				i += i;
				r = (long *)(v->v.t.rowlist+i);
				v->cursaddr += r[0] - r[-1];
			}
		}
		break;
	case 'C':		/* cursor right */
		if (cx < v->maxx) {
			if ((i = v->planes-1) && (cx & 1))
				v->cursaddr += i + i;
			v->cx = ++cx;
			v->cursaddr++;
		}
		break;
	case 'D':		/* cursor left */
		if (cx) {
			v->cx = --cx;
			v->cursaddr--;
			if ((i = v->planes-1) && (cx & 1))
				v->cursaddr -= i + i;
		}
		break;
	case 'E':		/* clear home */
		clear(v);
		/* fall through... */
	case 'H':		/* cursor home */
		v->cx = 0; v->cy = 0;
		v->cursaddr = V_LINE(v, 0);
		break;
	case 'I':		/* cursor up, insert line */
		if (cy == 0) {
			insert_line(v, 0);
			if (!V_LINEAR_P(v)) {
				long *r;
				i = cy + cy;
				i += i;
				r = (long *)(v->v.t.rowlist+i);
				v->cursaddr -= r[1] - r[0];
			}
		}
		else
			goto moveup;
		break;
	case 'J':		/* clear below cursor */
		clrfrom(v, cx, cy, v->maxx, v->maxy);
		break;
	case 'K':		/* clear remainder of line */
		clrfrom(v, cx, cy, v->maxx, cy);
		break;
	case 'L':		/* insert a line */
		v->cx = 0;
		i = cy + cy;
		i += i;
		insert_line(v, cy);
		v->cursaddr = V_LINE(v, i);
		break;
	case 'M':		/* delete line */
		v->cx = 0;
		i = cy + cy;
		i += i;
		delete_line(v, cy);
		v->cursaddr = V_LINE(v, i);
		break;
	case 'Q':		/* EXTENSION: quote-next-char */
		*V_STATE(v) = quote_putch;
		return;
	case 'Y':
		*V_STATE(v) = escy_putch;
		return;		/* YES, this should be 'return' */

	case 'b':
		*V_STATE(v) = setfgcol;
		return;
	case 'c':
		*V_STATE(v) = setbgcol;
		return;
	case 'd':		/* clear to cursor position */
		clrfrom(v, 0, 0, cx, cy);
		break;
	case 'e':		/* enable cursor */
		v->flags |= CURS_ON;
		v->hidecnt = 1;	/* so --v->hidecnt shows the cursor */
		break;
	case 'f':		/* cursor off */
		v->hidecnt++;
		v->flags &= ~CURS_ON;
		break;
	case 'j':		/* save cursor position */
		v->savex = v->cx;
		v->savey = v->cy;
		break;
	case 'k':		/* restore saved position */
		gotoxy(v, v->savex, v->savey);
		break;
	case 'l':		/* clear line */
		v->cx = 0;
		i = cy + cy;
		i += i;
		v->cursaddr = V_LINE(v, i);
		clrline(v, cy);
		break;
	case 'o':		/* clear from start of line to cursor */
		clrfrom(v, 0, cy, cx, cy);
		break;
	case 'p':		/* reverse video on */
		v->flags |= FINVERSE;
		break;
	case 'q':		/* reverse video off */
		v->flags &= ~FINVERSE;
		break;
	case 't':		/* EXTENSION: set cursor flash rate */
		*V_STATE(v) = setcurs;
		return;
	case 'v':		/* wrap on */
		v->flags |= FWRAP;
		break;
	case 'w':
		v->flags &= ~FWRAP;
		break;
	case 'y':		/* EXTENSION: set special effects */
		*V_STATE(v) = seffect_putch;
		curs_on(v);
		return;
	case 'z':		/* EXTENSION: clear special effects */
		*V_STATE(v) = ceffect_putch;
		curs_on(v);
		return;
	}
	*V_STATE(v) = normal_putch;
}

/*
 * escy1_putch(v, c): for when an ESC Y + char has been seen
 */
static void
escy1_putch(v, c)
	SCREEN *v;
	int c;
{
	/* some (un*x) termcaps seem to always set the hi bit on
	   cm args (cm=\EY%+ %+ :) -> drop that unless the screen
	   is bigger.	-nox
	*/
	gotoxy(v, (c-' ') & (v->maxx|0x7f), (V_ESCY1(v)-' ') & (v->maxy|0x7f));
	*V_STATE(v) = normal_putch;
}

/*
 * escy_putch(v, c): for when an ESC Y has been seen
 */
static void
escy_putch(v, c)
	SCREEN *v;
	int c;
{
	V_ESCY1(v) = c;
	*V_STATE(v) = escy1_putch;
}

/*
 * normal_putch(v, c): put character 'c' on screen 'v'. This is the default
 * for when no escape, etc. is active
 */

static void
normal_putch(v, c)
	SCREEN *v;
	int c;
{
	register int i;

/* control characters */
	if (c < ' ') {
		switch (c) {
		case '\r':
col0:			v->cx = 0;
			i = v->cy + v->cy;
			i += i;
			v->cursaddr = V_LINE(v, i);
			return;
		case '\n':
			if (v->cy == v->maxy) {
				scroll(v);
				if (!V_LINEAR_P(v)) {
					long *r;
					i = v->cy + v->cy;
					i += i;
					r = (long *)(v->v.t.rowlist+i);
					v->cursaddr += r[0] - r[-1];
				}
			} else {
				v->cy++;
				if (V_LINEAR_P(v))
					v->cursaddr += v->linelen;
				else {
					long *r;
					i = v->cy + v->cy;
					i += i;
					r = (long *)(v->v.t.rowlist+i);
					v->cursaddr += r[0] - r[-1];
				}
			}
			return;
		case '\b':
			if (v->cx) {
				v->cx--;
				v->cursaddr--;
				if ((i = v->planes-1) && (v->cx & 1))
					v->cursaddr -= i+i;
			}
			return;
		case '\007':		/* BELL */
			(void)bconout(CONDEV, 7);
			return;
		case '\033':		/* ESC */
			*V_STATE(v) = putesc;
			return;
		case '\t':
			if (v->cx < v->maxx) {
			/* this can't be register for an ANSI compiler */
				union {
					long l;
					short i[2];
				} j;
				j.l = 0;
				j.i[1] = 8 - (v->cx & 7);
				v->cx += j.i[1];
				if (v->cx - v->maxx > 0) {
					j.i[1] = v->cx - v->maxx;
					v->cx = v->maxx;
				}
				v->cursaddr += j.l;
				if ((i = v->planes-1)) {
					if (j.l & 1)
						j.i[1]++;
					do v->cursaddr += j.l;
					while (--i);
				}
			}
			return;
		default:
			return;
		}
	}

	(*vpaint)(v, c, v->cursaddr);
	v->cx++;
	if (v->cx > v->maxx) {
		if (v->flags & FWRAP) {
			normal_putch(v, '\n');
			goto col0;
		} else {
			v->cx = v->maxx;
		}
	} else {
		v->cursaddr++;
		if ((i = v->planes-1) && !(v->cx & 1))	/* new word */
			v->cursaddr += i + i;
	}
}

INLINE static void
put_ch00(v, c)
	SCREEN *v;
	int c;
{
	(*v00state)(v, c & 0x00ff);
}

INLINE static void
put_ch0x(v, c)
	SCREEN *v;
	int c;
{
	(*v->v.t.state)(v, c & 0x00ff);
}

static long ARGS_ON_STACK screen_open	P_((FILEPTR *f));
static long ARGS_ON_STACK screen_read	P_((FILEPTR *f, char *buf, long nbytes));
static long ARGS_ON_STACK screen_write P_((FILEPTR *f, const char *buf, long nbytes));
static long ARGS_ON_STACK screen_lseek P_((FILEPTR *f, long where, int whence));
static long ARGS_ON_STACK screen_ioctl P_((FILEPTR *f, int mode, void *buf));
static long ARGS_ON_STACK screen_close P_((FILEPTR *f, int pid));
static long ARGS_ON_STACK screen_select P_((FILEPTR *f, long p, int mode));
static void ARGS_ON_STACK screen_unselect P_((FILEPTR *f, long p, int mode));

static long ARGS_ON_STACK screen_datime	P_((FILEPTR *f, short *time, int rwflag));

DEVDRV vcon_device = {
	screen_open, screen_write, screen_read, screen_lseek, screen_ioctl,
	screen_datime, screen_close, screen_select, screen_unselect
};

static long ARGS_ON_STACK 
screen_open(f)
	FILEPTR *f;
{
	int fd, vt = f->fc.aux;
	char name[] = "u:\\pipe\\q$vt00";

	if (!rowoff) {
		init();
	} else if (!ttys[0].use_cnt || leaving)
		/* if we're init'ed already and vt00 is closed that means
		   we're uninistalling... */
		return -EACCESS;
	if (!((struct tty *)f->devinfo)->use_cnt) {
		SCREEN *v = VT_SCREEN(vt);

		/* init and alloc screen memory if necessary */
		if (vt) {
			if (!v->v.t.vbase) {
				char *vbase = (char *)kmalloc(scrnsize);
				if (!vbase)
					return -ENOMEM;
				init_screen (v, vbase, v->v.t.rowlist, 0);
			} else if (v->v.t.on == V_FREE)
				v->v.t.on = V_USED;
		}
		/* is there a better way??? */
		name[sizeof "u:\\pipe\\q$vt0"-1] = vt+'0';
		if ((fd = FOPEN (name, O_RDONLY|O_GLOBAL)) < 0)
			return fd;
		qfd[vt] = fd;
		q_fl[vt] = 0;
	}

	f->flags |= O_TTY;
	return 0;
}

static long ARGS_ON_STACK 
screen_close(f, pid)
	FILEPTR *f;
	int pid;
{
	UNUSED(pid);

	if (!((struct tty *)f->devinfo)->use_cnt) {
		int vt = f->fc.aux;

		/* close pipe */
		FCLOSE (qfd[vt]);

		/* last close on vt00 means uninstall... */
		if (!vt)
			deinit();
		/* otherwise it means free screen memory */
		else {
			SCREEN *v = VT_SCREEN(vt);

			if (v->v.t.on)
				v->v.t.on = V_FREE;
			else {
				kfree (v->v.t.vbase);
				v->v.t.vbase = 0;
			}
		}
	}
	return 0;
}

static long ARGS_ON_STACK 
screen_write(f, buf, bytes)
	FILEPTR *f; const char *buf; long bytes;
{
	int vt = f->fc.aux;
	SCREEN *v = VT_SCREEN(vt);
	long *r;
	long ret = 0;
	int c;
	long tick;

	UNUSED(f);

	/* tty_write is calling us with no more than one line or 128
	   chars at a time but still never(?) allows task-switches
	   while doing a longer write... checkkeys() does this when
	   it detects a keyboard interrupt but we cant call that.
	   instead we look for 0->1 (_hz_200 & 3) ticks that happened
	   while we were writing (there are 50 of them in a second)
	   and yield() when found one.  (comments?)
	*/
#if 0
	(void)checkkeys();
#else
	tick = _hz_200;
#endif
	v->hidecnt++;
	v->flags |= CURS_UPD;		/* for TOS 1.0 */
	curs_off(v);
	r = (long *)buf;
	if (vt) {
		while (bytes > 0) {
			c = (int) *r++;
			put_ch0x(v, c);
			bytes -= 4; ret+= 4;
		}
	} else {
		while (bytes > 0) {
			c = (int) *r++;
			put_ch00(v, c);
			bytes -= 4; ret+= 4;
		}
	}
	if (v->hidecnt > 0)
		--v->hidecnt;
	else
		v->hidecnt = 0;
	curs_on(v);
	v->flags &= ~CURS_UPD;
#if 1
	if (tick != _hz_200 && !(tick & 3))
		yield();
#endif
	return ret;
}

static long ARGS_ON_STACK 
screen_read(f, buf, bytes)
	FILEPTR *f; char *buf; long bytes;
{
	int vt = f->fc.aux;

	if ((f->flags & O_NDELAY) != q_fl[vt])
		FCNTL (qfd[vt], (long)(q_fl[vt] = f->flags&O_NDELAY), F_SETFL);
	return FREAD (qfd[vt], bytes, buf);
}

static long ARGS_ON_STACK 
screen_lseek(f, where, whence)
	FILEPTR *f;
	long where;
	int whence;
{
/* terminals always are at position 0 */
	UNUSED(f); UNUSED(where);
	UNUSED(whence);
	return 0;
}

static long ARGS_ON_STACK 
screen_ioctl(f, mode, buf)
	FILEPTR *f; int mode; void *buf;
{
	int vt = f->fc.aux;
	long *r = (long *)buf;
	struct winsize *w;

	UNUSED(f);

	if (mode == FIONREAD) {
		*r = FINSTAT (qfd[vt]);
		if (*r > 0)
			*r >>= 2;
	}
	else if (mode == FIONWRITE) {
		*r = 0x400;
	}
	else if (mode == TIOCFLUSH) {
		return FCNTL (qfd[vt], r, TIOCFLUSH);
	}
	else if (mode == TIOCGWINSZ) {
		SCREEN *v = VT_SCREEN(vt);
		w = (struct winsize *)buf;
		w->ws_row = v->maxy+1;
		w->ws_col = v->maxx+1;
	}
	else if (mode >= TCURSOFF && mode <= TCURSGRATE) {
		SCREEN *v = VT_SCREEN(vt);
		switch(mode) {
		case TCURSOFF:
			curs_off(v);
			v->hidecnt++;
			v->flags &= ~CURS_ON;
			break;
		case TCURSON:
			v->flags |= CURS_ON;
			v->hidecnt = 0;
			curs_on(v);
			break;
		case TCURSBLINK:
			curs_off(v);
			v->flags |= CURS_FLASH;
			curs_on(v);
			break;
		case TCURSSTEADY:
			curs_off(v);
			v->flags &= ~CURS_FLASH;
			curs_on(v);
			break;
		case TCURSSRATE:
			v->period = *((short *)buf);
			break;
		case TCURSGRATE:
			return v->period;
		}
	} else
		return -EINVAL;

	return 0;
}

static long ARGS_ON_STACK 
screen_select(f, p, mode)
	FILEPTR *f; long p; int mode;
{
	struct tty *tty = (struct tty *)f->devinfo;
	int vt = f->fc.aux;

	if (mode == O_RDONLY) {
		if (FINSTAT (qfd[vt])) {
			return 1;
		}
		if (tty) {
		/* avoid collisions with other processes */
			if (!tty->rsel)
				tty->rsel = p;
		}
		return 0;
	} else if (mode == O_WRONLY) {
		return 1;
	}
	/* default -- we don't know this mode, return 0 */
	return 0;
}

static void ARGS_ON_STACK 
screen_unselect(f, p, mode)
	FILEPTR *f;
	long p;
	int mode;
{
	struct tty *tty = (struct tty *)f->devinfo;

	if (tty) {
		if (mode == O_RDONLY && tty->rsel == p)
			tty->rsel = 0;
		else if (mode == O_WRONLY && tty->wsel == p)
			tty->wsel = 0;
	}
}

long ARGS_ON_STACK 
screen_datime(f, timeptr, rwflag)
	FILEPTR *f;
	short *timeptr;
	int rwflag;
{
	int vt = f->fc.aux;

	if (rwflag)
		return -EACCESS;
	return FDATIME (timeptr, qfd[vt], 0);
}
