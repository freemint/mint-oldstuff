/*
virtual terminal devices, based on MiNTs fasttext.c

Some parts of this code are:
Copyright 1992,1993 Eric R. Smith and Atari Corporation.
Used by permission.

compile with -DVT00XCON to make ttyv0 (the `new' console) output thru
xconout[2] (for GEM programs that don't know about ptys and hook up
their terminal window there...), otherwise ttyv0 is always fast
full-screen like the other terminals only it doesn't hardware scroll.

compile with -DVMODE to allow (hardware dependent) different video modes
for ttyv1..9 and console, see screen.c for details.

use -DFORCE1PLANE to compile only code for one plane (i.e. no colour)
and characters 8 or 16 bytes high, this is fastest because it can leave
out a few checks and inline the paint code.  of course that needs either
a monochrome screen or screen.c (showscreen) has to know how to set/save
palette 1 for ttyv1..9.  and it can't do console writes -> implies VT00XCON.
*/

#include <stddef.h>
#include <errno.h>
#include <osbind.h>
#include "vcon.h"
#include "vtdev.h"

#ifdef FORCE1PLANE
#include "paint.c"
#endif

#define CONDEV	(2)

#define VT_SCREEN(vt) (v0x+(vt)-1)
#define TT_TTYV(tty) ((struct ttyv *) \
			((char *)(tty)-offsetof(struct ttyv, tt)))
#define TT_SCREEN(tty) (TT_TTYV(tty)->v)
#define SCNSIZE(v) ( (((long)v->maxy + hardscroll + 2)) * v->linelen )

SCREEN *v00, v0x[N_VT-1];
char *chartab[256*2];
short hardscroll;
long scrnsize;
char *rowoff;
char *sysfontdata;	/* saved ttyv[1..9] v->fontdata if using loaded font */
long sfontbytes;	/* size of ... */
char *sysfontdata0;	/* saved console v->fontdata if using loaded font */
#ifndef FORCE1PLANE
void (*vpaint) P_((SCREEN *, int, char *));
#endif
#ifndef VT00XCON
void (*vpaint0x) P_((SCREEN *, int, char *));
#endif

static char *hardbase;
static short hardline;
static short qfd[N_VT], q_fl[N_VT];

INLINE static void curs_off P_((SCREEN *)), curs_on P_((SCREEN *));
static void normal_putch P_((SCREEN *, int));
static void escy_putch P_((SCREEN *, int));
static void quote_putch P_((SCREEN *, int));
INLINE static void exchangeb P_((void *, void *, long));
INLINE static int init P_((void));
INLINE static void deinit P_((void));
INLINE static int setcurrent P_((int));
INLINE static void hardware_scroll P_((SCREEN *));
INLINE static void hardware_scroll_down P_((SCREEN *));
INLINE static void gotoxy P_((SCREEN *, int, int));
INLINE static void clrfrom P_((SCREEN *, int, int, int, int));
INLINE static void delete_line P_((SCREEN *, int));
INLINE static void insert_line P_((SCREEN *, int));
static void setbgcol P_((SCREEN *, int));
static void setfgcol P_((SCREEN *, int));
static void setcurs P_((SCREEN *, int));
static void setcshape P_((SCREEN *, int));
static void putesc P_((SCREEN *, int));
static void escy1_putch P_((SCREEN *, int));
#ifdef VT00XCON
static long xconout_start;
#else
int fgmask[MAX_PLANES], bgmask[MAX_PLANES], fgff, bg00;
static Vfunc v00state;
INLINE static void put_ch00 P_((SCREEN *, int));
#endif
INLINE static void put_ch0x P_((SCREEN *, int));

/* actually flash cursor (called from vcon.c) */

INLINE static void
xflash()
{
	SCREEN *v = VT_SCREEN(vcurrent);

	/* ttyv0's cursor is handled by TOS... */
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
#ifdef VT00XCON
			if ((v->flags & CURS_FLASH) &&
			    (!vcurrent || !v->v.t.on))
#else
			if ((v->flags & CURS_FLASH) &&
			    v != v00 && (!vcurrent || !v->v.t.on))
#endif
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

/* init ttyv[1-9] SCREEN struct */

void
init_screen(v, initv, vbase, rowlist, on)
	SCREEN *v, *initv;
	char *vbase, *rowlist;
	short on;
{
	static char initv00[sizeof (SCREEN) - offsetof (SCREEN, cheight)];
#ifndef FORCE1PLANE
	static int iusedplanes;
#endif

	if (initv) {
		memmove (initv00, (char *)&initv->cheight, sizeof (initv00));
#ifndef FORCE1PLANE
		if (initv != v00 && initv->v.t.usedplanes)
			iusedplanes = initv->v.t.usedplanes;
#endif
	}
	bzero ((char *)v, offsetof (SCREEN, cheight));
	memmove ((char *)&v->cheight, initv00, sizeof (initv00));

#ifndef FORCE1PLANE
	if (iusedplanes)
		v->v.t.usedplanes = iusedplanes;
	else
		v->v.t.usedplanes = v->planes;
#endif
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
setup_chartab(v)
	SCREEN *v;
{
	int i, j;
	char *data, *foo;
	static char chardata[256*16*2];

#ifndef FORCE1PLANE
	if (v->cheight == 8 && V_USEDPLANES(v) == 2) {
		vpaint = paint8c;
		foo = &chardata[0];
		for (i = 0; i < 256; i++) {
			chartab[i] = foo;
			data = v->fontdata + i;
			for (j = 0; j < 8; j++) {
				*foo++ = *data;
				data += v->form_width;
			}
		}
		for (i = 0; i < 256; i++) {
			chartab[i+256] = foo;
			data = v->fontdata + i;
			for (j = 0; j < 8; j++) {
				unsigned char d = *data;

				d |= d >> 1;
				*foo++ = d;
				data += v->form_width;
			}
		}
	} else if ((v->cheight == 16 || v->cheight == 8) &&
			V_USEDPLANES(v) == 1) {
		vpaint = paint816m;
#endif
		foo = &chardata[0];
		for (i = 0; i < 256; i++) {
			chartab[i] = foo;
			data = v->fontdata + i;
			for (j = 0; j < v->cheight; j++) {
				*foo++ = *data;
				data += v->form_width;
			}
		}
		for (i = 0; i < 256; i++) {
			chartab[i+256] = foo;
			data = v->fontdata + i;
			for (j = 0; j < v->cheight; j++) {
				unsigned char d = *data;

				d |= d >> 1;
				*foo++ = d;
				data += v->form_width;
			}
		}
#ifndef FORCE1PLANE
	}
	else
		vpaint = paint;
#endif
}

INLINE static int
init()
{
	SCREEN *v;
	int i;
	char *foo;
	register int linelen;
	long sfontbytes0;

	foo = lineA0();
	v = getvtmode (ttys[0].v = v00 = (SCREEN *)(foo - 346));
#ifdef FORCE1PLANE
	if ((v->cheight != 16 && v->cheight != 8) ||
		((v != v00 && v->v.t.usedplanes) ?
			v->v.t.usedplanes : v->planes) != 1) {
		ALERT("Colour and cheight != 8 or 16 not supported, recompile without -DFORCE1PLANE");
		return -1;
	}
#endif
	
	/* Ehem... The screen might be bigger than 32767 bytes.
	   Let's do some casting... 
	   Erling
	*/
	linelen = v->linelen;
	scrnsize = (v->maxy+1)*(long)linelen;
	sfontbytes = V_NFONTBYTES(v);
	sfontbytes0 = V_NFONTBYTES(v00) * 2;
	sysfontdata0 = v00->fontdata;
	rowoff = (char *)kmalloc((long)((v->maxy+1) * sizeof(long) * (N_VT-1)) + sfontbytes0);
	if (rowoff == 0) {
		ALERT("Insufficient memory for screen offset table!");
		return -ENOMEM;
	} else {
		long off, *lptr = (long *)rowoff;
		SCREEN *vp = v0x;

		for (i=0, off=0; i<=v->maxy; i++) {
			*lptr++ = off;
			off += linelen;
		}
		for (i=1; i<N_VT-1; i++) {
			ttys[i].v = vp++;
			vp->v.t.rowlist = (char *)lptr;
			lptr += v->maxy+1;
		}
		ttys[N_VT-1].v = vp;
		ttys[0].loadfontdata = (char *)lptr;
		ttys[0].loadfontbytes = sfontbytes0;
	}
	if (hardscroll == -1) {
	/* request for auto-setting */
		hardscroll = v->maxy+1;
	}
	if (!hardbase && (v == v00 || !(hardbase = v->v.t.vbase))) {
		long scnsize = SCNSIZE(v);

		hardbase = (char *)(((long)kcore(scnsize+256L+sfontbytes)+255L)
					   & 0xffffff00L);
		if (hardbase == 0) {
			ALERT("Insufficient memory for second screen buffer!");
			kfree (rowoff);
			return -ENOMEM;
		}
		ttys[1].loadfontdata = hardbase + scnsize;
		ttys[1].loadfontbytes = sfontbytes;
	}
	init_screen(v0x, v, hardbase, rowoff, V_FREE);
	hardline = 0;
	setup_chartab(v);
	sysfontdata = v->fontdata;

#ifndef VT00XCON
	vpaint0x = vpaint;
	v = v00;

	if (v->hidecnt == 0) {
	/*
	 * make sure the console cursor is set up correctly and turned on
	 */
		(void)Cursconf(0,0);	/* turn cursor off */

		v->flags &= ~CURS_FSTATE;

	/* now turn the cursor on the way we like it */
		v->curstimer = v->period;
		v->hidecnt = 0;
		v->flags |= CURS_ON;
		vpaint = paint;
		curs_on(v);
		vpaint = vpaint0x;
	} else {
		(void)Cursconf(0,0);
		v->flags &= ~CURS_ON;
		v->hidecnt = 1;
	}

	/* setup bgmask and fgmask */
	setbgcol(v, v->bgcol);
	setfgcol(v, v->fgcol);
	*V_STATE(v) = normal_putch;
#endif
	return 0;
}

/* deinit, must be called after last close */

INLINE static void
deinit()
{
	kfree (rowoff);
}

/* exchange memory, assumes pointers word-aligned and bytes
   multiple of sizeof long...  (faster implementations welcome :-)
*/

INLINE static
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

INLINE static int
setcurrent(vt)
	int vt;
{
	static int v0xcurrent = 1;
	SCREEN *v = VT_SCREEN(vt);
	int save;

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
		/* pointers... */
		foo = oldv->v.t.vbase;
		oldv->v.t.vbase = v->v.t.vbase;
		v->v.t.vbase = foo;
		foo = oldv->v.t.rowlist;
		oldv->v.t.rowlist = v->v.t.rowlist;
		v->v.t.rowlist = foo;
		/* and fonts... */
		exchangeb (ttys[v0xcurrent].loadfontdata,
				ttys[vt].loadfontdata, sfontbytes);
		foo = ttys[v0xcurrent].loadfontdata;
		ttys[v0xcurrent].loadfontdata = ttys[vt].loadfontdata;
		ttys[vt].loadfontdata = foo;
		if (v->fontdata != sysfontdata)
			v->fontdata = ttys[vt].loadfontdata;
		setup_chartab(v);

		/* free screen memory if told so */
		if (oldv->v.t.on == V_FREE) {
			oldv->v.t.on = 0;
			kfree (oldv->v.t.vbase);
			oldv->v.t.vbase = 0;
		} else {
			oldv->v.t.on = 0;
			oldv->cursaddr = PLACE(oldv, oldv->cx, oldv->cy);
			if (oldv->fontdata != sysfontdata)
				oldv->fontdata = ttys[v0xcurrent].loadfontdata;
		}
		v->v.t.on = V_USED;
		v->cursaddr = PLACE(v, v->cx, v->cy);
		v0xcurrent = vt;
	}
	save = !vcurrent;
	vcurrent = vt;
	if (vt && (v->flags & CURS_FLASH))
		curs_on(v);
	showscreen (vt, (vt ? v: v00), (vt ? v->v.t.vbase : base), save);
	return 0;
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
INLINE static void
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
		showscreen (vcurrent, v, v->v.t.vbase, 0);
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
#ifdef VT00XCON
		if (hardscroll > 0)
#else
		if (v != v00 && hardscroll > 0)
#endif
		{
			hardware_scroll(v);
			clrline(v, v->maxy);
			return;
		}
		nbytes = V_SCRNSIZE(v) - v->linelen;
	} else {
		register int i = v->maxy - r;
		i += i;
		i += i;
		nbytes = V_LINEOFF(v, i);
	}

	/* Sheeze, how many times do we really have to cast... 
	   Erling.	
	*/

	r += r;
	r += r;
	dst = (long *) V_LINE(v, r);
	src = (long *)( ((long)dst) + v->linelen);

	quickmove(dst, src, nbytes);

/* clear the last line */
	clrline(v, v->maxy);
}

INLINE static void
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
		showscreen (vcurrent, v, v->v.t.vbase, 0);
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
#ifdef VT00XCON
	if (!r && hardscroll > 0)
#else
	if (!r && v != v00 && hardscroll > 0)
#endif
	{
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
	src = (long *) V_LINE(v, i);
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
#ifdef FORCE1PLANE
	*V_BGMASK(v) = (v->bgcol = c & 1) ? -1 : 0;
#else
	int *m = V_BGMASK(v);
	int i, vplanes = V_USEDPLANES(v);

	v->bgcol = c & ((1 << vplanes)-1);
	*V_BG00(v) = !v->bgcol;
#ifndef VT00XCON
	if (v == v00)
		memset (m, ((v->bgcol == ((1 << vplanes)-1)) ? -1 : 0),
			MAX_PLANES * sizeof (*m));
#endif
	for (i = 0; i < vplanes; i++)
	    *m++ = (v->bgcol & (1 << i)) ? -1 : 0;
#endif
	*V_STATE(v) = normal_putch;
}

static void
setfgcol(v, c)
	SCREEN *v;
	int c;
{
#ifdef FORCE1PLANE
	*V_FGMASK(v) = (v->fgcol = c & 1) ? -1 : 0;
#else
	int *m = V_FGMASK(v);
	int i, vplanes = V_USEDPLANES(v);

	v->fgcol = c & ((1 << vplanes)-1);
	*V_FGFF(v) = v->fgcol == ((1 << vplanes)-1);
#ifndef VT00XCON
	if (v == v00)
		memset (m, (*V_FGFF(v) ? -1 : 0),
			MAX_PLANES * sizeof (*m));
#endif
	for (i = 0; i < vplanes; i++)
	    *m++ = (v->fgcol & (1 << i)) ? -1 : 0;
#endif
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

/* set cursor shape, bit 0 = blink(0)/steady(1), bit 1..? = shape
 * (0 = underline, 1 = block, 2..? unused; ignored on console)
 */
static void
setcshape(v, c)
	SCREEN *v;
	int c;
{
	if (c & 1) {
		v->flags &= ~CURS_FLASH;
		--c;
	} else {
		v->flags |= CURS_FLASH;
	}
#ifndef VT00XCON
	if (v != v00)
#endif
		v->v.t.cshape = c - ' ';
	*V_STATE(v) = normal_putch;
}

/* set special effects...  FIXME: still ignores light/italic */
static void
seffect_putch(v, c)
	SCREEN *v;
	int c;
{
	v->flags |= ((c & 0x10) ? FINVERSE : 0)|
			((c & 0x8) ? FUNDERLINE : 0)|
			((c & 0x1) ? FBOLD : 0);
	*V_STATE(v) = normal_putch;
}

/* clear special effects */
static void
ceffect_putch(v, c)
	SCREEN *v;
	int c;
{
	v->flags &= ~(((c & 0x10) ? FINVERSE : 0)|
			((c & 0x8) ? FUNDERLINE : 0)|
			((c & 0x1) ? FBOLD : 0));
	*V_STATE(v) = normal_putch;
}

static void
quote_putch(v, c)
	SCREEN *v;
	int c;
{
#ifdef FORCE1PLANE
	paint816m(v, c, v->cursaddr);
#else
	(*vpaint)(v, c, v->cursaddr);
#endif
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
	case '.':		/* EXTENSION: set cursor shape */
		*V_STATE(v) = setcshape;
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
	case '(':		/* EXTENSION: boldface on */
		v->flags |= FBOLD;
		break;
	case ')':		/* EXTENSION: boldface off */
		v->flags &= ~FBOLD;
		break;
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
	   cm args (cm=\EY%+ %+ ) -> drop that unless the screen
	   is bigger.	-nox
	*/
	gotoxy(v, (c-' ') & (v->maxx|0x7f), (*V_ESCY1(v)-' ') & (v->maxy|0x7f));
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
	*V_ESCY1(v) = c;
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
			if (v->cx <= v->maxx) {
			/* this can't be register for an ANSI compiler */
				union {
					long l;
					short i[2];
				} j;
				j.l = 0;
				j.i[1] = 8 - (v->cx & 7);
				v->cx += j.i[1];
				if (v->cx - v->maxx > 0) {
					j.i[1] -= v->cx - v->maxx;
					v->cx = v->maxx;
					if (v->flags & FWRAP) {
						normal_putch(v, '\n');
						goto col0;
					}
					if (j.i[1] <= 0)
						return;
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

#ifdef FORCE1PLANE
	paint816m(v, c, v->cursaddr);
#else
	(*vpaint)(v, c, v->cursaddr);
#endif
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

#ifdef VT00XCON
#ifdef __GNUC__
/* macro to call thru a xcon* vector.  we can tell gcc directly that it
   clobbers d0-d7 and a0-a5, only the frame pointer in a6 we must take
   care of ourselves.
   correction: gcc 2.2.2 also wants the reg.s it passes args in unchanged */

#define xcon_exec(add, ch) \
({									\
	register long retvalue __asm__("d0");				\
	register long  _add __asm__("a0") = (long) (add);		\
	long  _ch  = (long) (ch);					\
	    								\
	__asm__ volatile						\
	("\
		movml   a5-a6/d7,sp@-;					\
		movl    %2,sp@-;					\
		subl    a5,a5;			/* TOS 1.(0)4 bug */	\
		jsr	%1@;						\
		addql	#4,sp;						\
		movml   sp@+,a5-a6/d7; "				\
	: "=r"(retvalue)			/* outputs */		\
	: "a"(_add), "d"(_ch)		        /* inputs  */		\
	: "d1", "d2", "d3", "d4", "d5", "d6",				\
	  "a0", "a1", "a2", "a3", "a4"		/* clobbered regs */	\
	);								\
	retvalue;							\
})
#endif
#else
INLINE static void
put_ch00(v, c)
	SCREEN *v;
	int c;
{
	(*v00state)(v, c & 0x00ff);
}
#endif

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

static long ARGS_ON_STACK screen_writeb P_((FILEPTR *f, const char *buf, long nbytes));

DEVDRV vcon_device = {
	screen_open, screen_write, screen_read, screen_lseek, screen_ioctl,
	screen_datime, screen_close, screen_select, screen_unselect,
#ifdef WRITEB111
	screen_writeb
#else
	0
#endif
};

static long ARGS_ON_STACK 
screen_open(f)
	FILEPTR *f;
{
	int fd, vt = f->fc.aux;
	char name[] = "u:\\pipe\\q$ttyv0";

	if (!rowoff) {
#ifdef VT00XCON
		/* TOS <= 1.(0)0 didn't have xconout */
		if (os_version > 0x100)
			xconout_start = xconout[CONDEV];
#endif
		if ((fd = init()))
			/* pass error... */
			return fd;
	} else if (!ttys[0].tt.use_cnt || leaving)
		/* if we're init'ed already and ttyv0 is closed that means
		   we're uninistalling... */
		return -EACCESS;
	if (!((struct tty *)f->devinfo)->use_cnt) {
		/* init and alloc screen memory if necessary */
		if (vt) {
			struct ttyv *tt = TT_TTYV((struct tty *)f->devinfo);
			SCREEN *v = tt->v;

			if (!v->v.t.vbase) {
				char *vbase = (char *)kmalloc(scrnsize+sfontbytes);

				if (!vbase)
					return -ENOMEM;
				tt->loadfontdata = vbase + scrnsize;
				tt->loadfontbytes = sfontbytes;
				init_screen (v, (void *)0, vbase, v->v.t.rowlist, 0);
			} else if (v->v.t.on == V_FREE)
				v->v.t.on = V_USED;
		}
		/* is there a better way??? */
		name[sizeof "u:\\pipe\\q$ttyv"-1] = vt+'0';
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

		/* last close on ttyv0 means uninstall... */
		if (!vt) {
			ttys[0].v->fontdata = sysfontdata0;
			deinit();
		/* otherwise it means free screen memory */
		} else {
			struct ttyv *tt = TT_TTYV((struct tty *)f->devinfo);
			SCREEN *v = tt->v;
			char *p = tt->readxlat;
			int j;

			/* reset font... */
			v->fontdata = sysfontdata;
			/* ..and read translation */
			for (j = 0x80; j < 0x100; ++j) {
				*p++ = j;
			}
			if (v->v.t.on)
				v->v.t.on = V_FREE;
			else {
				kfree (v->v.t.vbase);
				v->v.t.vbase = 0;
				tt->loadfontbytes = 0;
				tt->loadfontdata = 0;
			}
		}
	}
	return 0;
}

#define _hz_200 (*((long *)0x4baL))

#ifdef WRITEB111
#define CHECKSLEEP (tick != _hz_200 && !(tick & 3))

static long ARGS_ON_STACK 
screen_write(f, buf, bytes)
	FILEPTR *f; const char *buf; long bytes;
{
	if (!bytes)
		return bytes;
	if (bytes != 4)
		ALERT("ttyv%c write bytes != 4, use MiNT >= 1.11 or recompile without -DWRITEB111",
			(char)f->fc.aux+'0');
	screen_writeb(f, buf+3, 1);
	return 4L;
}

static long ARGS_ON_STACK 
screen_writeb(f, buf, bytes)
	FILEPTR *f; const char *buf; long bytes;
#else
#define CHECKSLEEP 0

static long ARGS_ON_STACK 
screen_write(f, buf, bytes)
	FILEPTR *f; const char *buf; long bytes;
#endif
{
	int vt = f->fc.aux;
	SCREEN *v;
#ifdef WRITEB111
	unsigned char *r = (unsigned char *)buf;
#else
	long *r = (long *)buf;
#endif
	long ret = bytes;
	int c;
	long tick;
	static long lastw;
	static int lastv;
	extern int __mint;

	/* tty_write is calling us with no more than one line or 128
	   chars at a time but still never(?) allows task-switches
	   while doing a longer write... checkkeys() does this when
	   it detects a keyboard interrupt but we cant call that.
	   instead we look for 0->1 (_hz_200 & 3) ticks that happened
	   while we were writing (there are 50 of them in a second)
	   and yield() when found one.  (comments?)
	*/
	v = TT_SCREEN((struct tty *)f->devinfo);
	while (bytes > 0) {
#ifdef WRITEB111
		while (((struct tty *)f->devinfo)->state & TS_HOLD)
			SLEEP(IO_Q, (long)&((struct tty *)f->devinfo)->state);
#endif
#if 0
		(void)checkkeys();
#else
		tick = _hz_200;
#endif
#ifdef VT00XCON
		if (vt) {
			v->hidecnt++;
			v->flags |= CURS_UPD;		/* for TOS 1.0 */
			curs_off(v);
			do {
				c = (int) *r++;
				put_ch0x(v, c);
			} while ((bytes -= sizeof *r) > 0 && !CHECKSLEEP);
			if (v->hidecnt > 0)
				--v->hidecnt;
			else
				v->hidecnt = 0;
			curs_on(v);
			v->flags &= ~CURS_UPD;
		} else {
			if (xconout_start) {
				do {
					c = (int) *r++;
					(void) xcon_exec (xconout[CONDEV], (unsigned char) c);
				} while ((bytes -= sizeof *r) > 0 && !CHECKSLEEP);
			} else {
				do {
					c = (int) *r++;
					(void) bconout(CONDEV, (unsigned char) c);
				} while ((bytes -= sizeof *r) > 0 && !CHECKSLEEP);
			}
		}
#else
		v->hidecnt++;
		v->flags |= CURS_UPD;		/* for TOS 1.0 */
		if (vt) {
			curs_off(v);
			do {
				c = (int) *r++;
				put_ch0x(v, c);
			} while ((bytes -= sizeof *r) > 0 && !CHECKSLEEP);
		} else {
			if (v->cheight != v0x->cheight ||
			    V_USEDPLANES(v) != v0x->v.t.usedplanes)
				vpaint = paint;
			curs_off(v);
			do {
				c = (int) *r++;
				put_ch00(v, c);
			} while ((bytes -= sizeof *r) > 0 && !CHECKSLEEP);
		}
		if (v->hidecnt > 0)
			--v->hidecnt;
		else
			v->hidecnt = 0;
		curs_on(v);
		v->flags &= ~CURS_UPD;
		vpaint = vpaint0x;
#endif
		if (bytes && !(((struct tty *)f->devinfo)->state & TS_HOLD))
			yield();
	}
#ifndef WRITEB111
	if (tick != _hz_200 && !(tick & 3))
		yield();
#endif
	if (ret > 0 && (vt != lastv || (_hz_200 - lastw) >= 100) &&
	    __mint > 0x109) {
		struct bios_file *b = (struct bios_file *)f->fc.index;
		lastw = _hz_200;
		lastv = vt;
		b->xattr.atime = b->xattr.mtime = TGETTIME();
		b->xattr.adate = b->xattr.mdate = TGETDATE();
	}
	return ret;
}

static long ARGS_ON_STACK 
screen_read(f, buf, bytes)
	FILEPTR *f; char *buf; long bytes;
{
	int vt = f->fc.aux;
	long ret;
	extern int __mint;

	if ((f->flags & O_NDELAY) != q_fl[vt])
		FCNTL (qfd[vt], (long)(q_fl[vt] = f->flags&O_NDELAY), F_SETFL);
	ret = FREAD (qfd[vt], bytes, buf);
	if (ret > 0 && __mint > 0x109) {
		struct bios_file *b = (struct bios_file *)f->fc.index;
		b->xattr.atime = TGETTIME();
		b->xattr.adate = TGETDATE();
	}
	return ret;
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
		SCREEN *v = TT_SCREEN((struct tty *)f->devinfo);
		w = (struct winsize *)buf;
		w->ws_row = v->maxy+1;
		w->ws_col = v->maxx+1;
#ifdef VT00XCON
	/* another compatibility hack for those `never heared of ptys'
	   GEM programs:  if the vector changed since we started the
	   console size is `unknown'...
	*/
		if (!vt && xconout[CONDEV] != xconout_start)
			w->ws_row = w->ws_col = 0;
#endif
	}
#ifdef VT00XCON
	else if (vt && mode >= TCURSOFF && mode <= TCURSGRATE)
#else
	else if (mode >= TCURSOFF && mode <= TCURSGRATE)
#endif
	{
		SCREEN *v = TT_SCREEN((struct tty *)f->devinfo);

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
#ifdef VT00XCON
	} else if ((mode >= TCURSOFF && mode <= TCURSSTEADY)) {
		return Cursconf(mode - TCURSOFF, 0);
	} else if ((mode >= TCURSSRATE && mode <= TCURSGRATE)) {
		long r;

		r = Cursconf(mode - TCURSOFF, *((short *)buf));
		if (r >= 0) {
			*(short *)buf = r;
			r = 0;
		}
		return r;
#endif
	} else if (mode >= VCTLSETV && mode <= VCTLWSEL && PGETPID() == pgrp) {
		switch(mode) {
		case VCTLSETV:
			return setcurrent ((long) buf);
		case VCTLFLASH:
			xflash ();
			return 0;
		case VCTLWSEL:
			if (ttys[(long) buf].tt.rsel) {
				WAKESELECT(ttys[(long) buf].tt.rsel);
#if 0
				ttys[(long) buf].tt.rsel = 0;
#endif
			}
		}
/* font setting stuff...  */
	} else if (mode >= TCGETFONTSIZE && mode <= TCSETFONTXLAT) {
		struct ttyv *tt = TT_TTYV((struct tty *)f->devinfo);
		SCREEN *v = tt->v;
		long bytes;

		if ((f->flags & O_RWMODE) == O_WRONLY)
			return -EPERM;		/* want read permission */
		switch(mode) {
		case TCGETFONTSIZE:		/* (one) chars bitmap size */
			*(short *)buf = v->form_width >> (8-3);
			((short *)buf)[1] = v->cheight;
			break;
		/* case TCSETFONTSIZE: */	/* (not yet) */
		case TCGETFONTCHRS:		/* char range in font */
			*(short *)buf = 0;
			((short *)buf)[1] = 0xff;
			break;
		/* case TCSETFONTCHRS: */	/* (not yet) */
		case TCGETFONTBITS:		/* bitmaps (old GDOS format) */
			if (!buf)
				return -EINVAL;
			/*FALLTHRU*/
		case TCSETFONTBITS:
			bytes = V_NFONTBYTES(v);

			if (mode == TCGETFONTBITS) {
				memmove (buf, v->fontdata, bytes);
			} else if (!buf) {
				/* default == sysfont */
				v->fontdata = vt ? sysfontdata : sysfontdata0;
			} else if (tt->loadfontbytes < bytes) {
				return -ENOMEM;
			} else {
				memmove (tt->loadfontdata, buf, bytes);
				v->fontdata = tt->loadfontdata;
				setup_chartab(v);
			}
			break;
		case TCGETFONTXLAT:		/* ST -> font char mapping */
			if (!buf) {		/*  for chars 0x80..0xff */
				return -EINVAL;
			} else {
				memmove (buf, tt->readxlat, 0x80);
			}
			break;
		case TCSETFONTXLAT:
			if (!buf) {
				/* default == none */
				char *p = tt->readxlat;
				int j;

				for (j = 0x80; j < 0x100; ++j) {
					*p++ = j;
				}
			} else {
				memmove (tt->readxlat, buf, 0x80);
			}
			break;
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
	extern int __mint;

	if (mode == O_RDONLY) {
		if (FINSTAT (qfd[vt])) {
			return 1;
		}
		if (tty) {
		/* avoid collisions with other processes */
			if (!tty->rsel)
				tty->rsel = p;
			else if (__mint > 0x109)
				return 2;
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
