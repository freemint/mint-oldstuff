/*
virtual terminal devices, based on MiNTs fasttext.c
paint code extracted for -funroll-loops...

Some parts of this code are:
Copyright 1992,1993 Eric R. Smith and Atari Corporation.
Used by permission.
*/

#ifndef FORCE1PLANE
#include "vcon.h"
#include "vtdev.h"
#endif

static char *lastfontdata;

/* flash(v): invert the character currently under the cursor */

#ifdef FORCE1PLANE
INLINE static
#endif
void
flash(v)
	SCREEN *v;
{
	char *place;
	ITYPE vplanesiz;
#ifndef FORCE1PLANE
	ITYPE i, j, vplanes;
#endif

	vplanesiz = v->planesiz;
	place = v->cursaddr;

	v->curstimer = v->period;
#ifndef FORCE1PLANE
	if (vpaint == paint816m) {
#endif
		if (V_CSHAPE(v) == CS_BLOCK) {
			/* block cursor */
			*place = ~*place;
			place += vplanesiz;
			*place = ~*place;
			place += vplanesiz;
			*place = ~*place;
			place += vplanesiz;
			*place = ~*place;
			place += vplanesiz;
			*place = ~*place;
			place += vplanesiz;
			*place = ~*place;
			place += vplanesiz;
			*place = ~*place;
			place += vplanesiz;
			*place = ~*place;
			if (v->cheight == 8)
				return;
			place += vplanesiz;
			*place = ~*place;
			place += vplanesiz;
			*place = ~*place;
			place += vplanesiz;
			*place = ~*place;
			place += vplanesiz;
			*place = ~*place;
			place += vplanesiz;
			*place = ~*place;
			place += vplanesiz;
			*place = ~*place;
			place += vplanesiz;
			*place = ~*place;
			place += vplanesiz;
			*place = ~*place;
			return;
		} else {
			/* underline cursor */
			place += v->linelen - vplanesiz;

			*place = ~*place;
			place -= vplanesiz;
			*place = ~*place;
			if (v->cheight == 8)
				return;
			place -= vplanesiz;
			*place = ~*place;
			return;
		}
#ifndef FORCE1PLANE
	}
	if (vpaint == paint8c) {
		if (V_CSHAPE(v) == CS_BLOCK) {
			/* block cursor */
			*place = ~*place;
			place[2] = ~place[2];
			place += vplanesiz;
			*place = ~*place;
			place[2] = ~place[2];
			place += vplanesiz;
			*place = ~*place;
			place[2] = ~place[2];
			place += vplanesiz;
			*place = ~*place;
			place[2] = ~place[2];
			place += vplanesiz;
			*place = ~*place;
			place[2] = ~place[2];
			place += vplanesiz;
			*place = ~*place;
			place[2] = ~place[2];
			place += vplanesiz;
			*place = ~*place;
			place[2] = ~place[2];
			place += vplanesiz;
			*place = ~*place;
			place[2] = ~place[2];
			return;
		} else {
			/* underline cursor */
			place += v->linelen - vplanesiz;

			*place = ~*place;
			place[2] = ~place[2];
			place -= vplanesiz;
			*place = ~*place;
			place[2] = ~place[2];
			return;
		}
	}
	vplanes = V_USEDPLANES(v);
	vplanes += vplanes;

	if (V_CSHAPE(v) == CS_BLOCK) {
		/* block cursor */
		for (j = v->cheight; j > 0; --j) {
			char *q = place;
			for (i = 0; i < vplanes; i+=2) {
				*q = ~*q;
				q+=2;
			}
			place += vplanesiz;
		}
	} else {
		/* underline cursor */
		place += v->linelen;

		for (j = (v->cheight>>3) + 1; j > 0; --j) {
			char *q;

			place -= vplanesiz;
			q = place;
			for (i = 0; i < vplanes; i+=2) {
				*q = ~*q;
				q+=2;
			}
		}
	}
#endif
}

#ifndef FORCE1PLANE
/*
 * effectbits(d): transform character bitmap, used in paint.
 */

INLINE static
unsigned char effectbits(v, d, j)
	SCREEN *v;
	unsigned char d;
	ITYPE j;
{	
	ITYPE l;
	if (v->flags & FITALIC) {
		l=j;
		d >>= l/((v->cheight)/3);
	}
	if (v->flags & FBOLD)
		d |= d >> 1; /* bold */
	if (v->flags & FDIM)
		d &= ((j % 2) ? 0x55 : 0xaa); /* dim */
	return d;
}

/*
 * paint(v, c, place): put character 'c' at position 'place' on screen
 * v. It is assumed that x, y are proper coordinates!
 * Specialized versions (paint8c and paint816m) of this routine follow;
 * they assume 8 line high characters, medium res. and 8 or 16 line/mono,
 * respectively.
 */

void
paint(v, c, place)
	SCREEN *v;
	int c;
	char *place;
{
	char *data, *q, d, doinverse;
	ITYPE j, planecount;
	int vplanes;
	long vform_width, vplanesiz;

	vplanes = V_USEDPLANES(v);
	vplanesiz = v->planesiz;
	doinverse = (v->flags & FINVERSE) ? 0xff : 0;
	if (c == ' ' && !(v->flags & FUNDERLINE) && *V_BG00(v) && *V_FGFF(v)) {
		for (j = v->cheight; j > 0; --j) {
			for (planecount = 0, q = place;
					planecount < vplanes; planecount++) {
				*q = doinverse;
				q+=2;
			}
			place += vplanesiz;
		}
		return;
	}

	data = v->fontdata + c;
	vform_width = v->form_width;

	if (*V_BG00(v) && *V_FGFF(v) && !(v->flags & FBOLD) && !(v->flags & FITALIC) && !(v->flags &FDIM)) {
		for (j = v->cheight-1; j > 0; --j) {
			d = *data ^ doinverse;
			for (planecount = 0, q = place;
					planecount < vplanes; planecount++) {
				*q = d;
				q+=2;
			}
			place += vplanesiz;
			data += vform_width;
		}
		d = ((v->flags & FUNDERLINE) ? -1 : *data) ^ doinverse;
		for (planecount = 0, q = place;
				planecount < vplanes; planecount++) {
			*q = d;
			q+=2;
		}
	} else {
		int *fg = V_FGMASK(v), *bg = V_BGMASK(v);

		for (j = v->cheight-1; j > 0; --j) {
			d = effectbits(v, *data, j) ^ doinverse;
			for (planecount = 0, q = place;
					planecount < vplanes; planecount++) {
				*q = ((d & (char) *fg++) | (~d & (char) *bg++));
				q+=2;
			}
			place += vplanesiz;
			data += vform_width;
			fg -= vplanes;
			bg -= vplanes;
		}
		d = ((v->flags & FUNDERLINE) ? -1 : effectbits(v, *data, (v->cheight-1)))
			^ doinverse;
		for (planecount = 0, q = place;
				planecount < vplanes; planecount++) {
			*q = ((d & (char) *fg++) | (~d & (char) *bg++));
			q+=2;
		}
	}
}

void
paint8c(v, c, place)
	SCREEN *v;
	int c;
	char *place;
{
	char *data;
	char d, doinverse, dounderline;
	long vplanesiz;

	vplanesiz = v->planesiz;
	doinverse = (v->flags & FINVERSE) ? 0xff : 0;
	if (c == ' ' && !(v->flags & FUNDERLINE) && *V_BG00(v) && *V_FGFF(v)) {
		ITYPE i;

		for (i = 8; i > 0; --i) {
			*place = doinverse;
			place[2] = doinverse;
			place += vplanesiz;
		}
		return;
	}
	if (v->flags & FBOLD)
		c += 0x100;
#ifdef V_LIGHT
	if (v->flags & FITALIC)
		c += 0x200;
#endif /* V_LIGHT */
	if (v->fontdata != lastfontdata) {
		setup_chartab(v);
		lastfontdata = v->fontdata;
	}
	data = chartab[c];
	dounderline = (v->flags & FUNDERLINE) ? 0xff : 0;

	if (!doinverse && *V_BG00(v) && *V_FGFF(v)) {
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
		char bg0, bg1, fg0, fg1;
		int *m = V_BGMASK(v);

		bg0 = *m++;
		bg1 = *m++;
		m = V_FGMASK(v);
		fg0 = *m++;
		fg1 = *m++;

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
#endif
 
#ifdef FORCE1PLANE
INLINE static
#endif
void
paint816m(v, c, place)
	SCREEN *v;
	int c;
	char *place;
{
	char *data;
#ifndef V_LIGHT
	char d, doinverse, dounderline;
#else
	char d, doinverse, dounderline, dodim;
#endif /* V_LIGHT */
	long vplanesiz;

	doinverse = (v->flags & FINVERSE) ? 0xff : 0;
	doinverse ^= (d = V_BGMASK(v)[0]);
	vplanesiz = v->planesiz;
#ifdef V_LIGHT
	dodim = (v->flags & FDIM) ? 0xff : 0;
#endif /* V_LIGHT */

	if (c == ' ' && !(v->flags & FUNDERLINE)) {
		d = doinverse;
		goto space;
	}
	if (v->flags & FBOLD)
		c += 0x100;
#ifdef V_LIGHT
	if (v->flags & FITALIC)
		c += 0x200;
#endif /* V_LIGHT */
	if (v->fontdata != lastfontdata) {
		setup_chartab(v);
		lastfontdata = v->fontdata;
	}
	data = chartab[c];
	dounderline = (v->flags & FUNDERLINE) ? 0xff : 0;

	if (d == V_FGMASK(v)[0]) {
space:
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
#ifndef V_LIGHT
	} else if (!doinverse) {
#else
	} else if (!doinverse && !dodim) {
#endif /* V_LIGHT */
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
#ifndef V_LIGHT
	} else {
#else
	} else if (doinverse && !dodim) {
#endif /* V_LIGHT */
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
			*place = d & ~dounderline;
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
		d = ~(*data | dounderline);
		*place = d;
#ifdef V_LIGHT
	} else if (!doinverse && dodim) {
          		/* line 1 */
		d = *data++ & 0xaa;
		*place = d;
		place += vplanesiz;

		/* line 2 */
		d = *data++ & 0x55;
		*place = d;
		place += vplanesiz;

		/* line 3 */
		d = *data++ & 0xaa;
		*place = d;
		place += vplanesiz;

		/* line 4 */
		d = *data++ & 0x55;
		*place = d;
		place += vplanesiz;

		/* line 5 */
		d = *data++ & 0xaa;
		*place = d;
		place += vplanesiz;

		/* line 6 */
		d = *data++ & 0x55;
		*place = d;
		place += vplanesiz;

		/* line 7 */
		d = *data++ & 0xaa;
		*place = d;
		place += vplanesiz;

		/* line 8 */
		d = *data++ & 0x55;
		if (v->cheight == 8) {
			*place = d | (dounderline & 0x55);
			return;
		}
		*place = d;

		place += vplanesiz;

		/* line 9 */
		d = *data++ & 0xaa;
		*place = d;
		place += vplanesiz;

		/* line 10 */
		d = *data++ & 0x55;
		*place = d;
		place += vplanesiz;

		/* line 11 */
		d = *data++ & 0xaa;
		*place = d;
		place += vplanesiz;

		/* line 12 */
		d = *data++ & 0x55;
		*place = d;
		place += vplanesiz;

		/* line 13 */
		d = *data++ & 0xaa;
		*place = d;
		place += vplanesiz;

		/* line 14 */
		d = *data++ & 0x55;
		*place = d;
		place += vplanesiz;

		/* line 15 */
		d = *data++ & 0xaa;
		*place = d;
		place += vplanesiz;

		/* line 16 */
		d = (*data | dounderline) & 0x55;
		*place = d;
	} else {
          		/* line 1 */
		d = ~(*data++ & 0xaa);
		*place = d;
		place += vplanesiz;

		/* line 2 */
		d = ~(*data++ & 0x55);
		*place = d;
		place += vplanesiz;

		/* line 3 */
		d = ~(*data++ & 0xaa);
		*place = d;
		place += vplanesiz;

		/* line 4 */
		d = ~(*data++ & 0x55);
		*place = d;
		place += vplanesiz;

		/* line 5 */
		d = ~(*data++ & 0xaa);
		*place = d;
		place += vplanesiz;

		/* line 6 */
		d = ~(*data++ & 0x55);
		*place = d;
		place += vplanesiz;

		/* line 7 */
		d = ~(*data++ & 0xaa);
		*place = d;
		place += vplanesiz;

		/* line 8 */
		d = ~(*data++ & 0x55);
		if (v->cheight == 8) {
			*place = d & ~(dounderline & 0x55);
			return;
		}
		*place = d;

		place += vplanesiz;

		/* line 9 */
		d = ~(*data++ & 0xaa);
		*place = d;
		place += vplanesiz;

		/* line 10 */
		d = ~(*data++ & 0x55);
		*place = d;
		place += vplanesiz;

		/* line 11 */
		d = ~(*data++ & 0xaa);
		*place = d;
		place += vplanesiz;

		/* line 12 */
		d = ~(*data++ & 0x55);
		*place = d;
		place += vplanesiz;

		/* line 13 */
		d = ~(*data++ & 0xaa);
		*place = d;
		place += vplanesiz;

		/* line 14 */
		d = ~(*data++ & 0x55);
		*place = d;
		place += vplanesiz;

		/* line 15 */
		d = ~(*data++ & 0xaa);
		*place = d;
		place += vplanesiz;

		/* line 16 */
		d = ~((*data | dounderline) & 0x55);
		*place = d;
#endif /* V_LIGHT */
	}

}

/*
 * clrline(v, r): clear line r of screen v
 */

#ifdef FORCE1PLANE
INLINE static
#endif
void
clrline(v, r)
	SCREEN *v;
	int r;
{
	int *dst;
#ifndef FORCE1PLANE
	int *m;
	long nwords;
	int i, vplanes = V_USEDPLANES(v);
#endif

	/* Hey, again the screen might be bigger than 32767 bytes.
	   Do another cast... */
	r += r;
	r += r;
	dst = (int *)(V_LINE(v, r));
#ifdef FORCE1PLANE
	memset ((char *)dst, *V_BGMASK(v), v->linelen);
#else
	if (*V_BG00(v))
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
#endif
}

/*
 * clear(v): clear the whole screen v
 */

#ifdef FORCE1PLANE
INLINE static
#endif
void
clear(v)
	SCREEN *v;
{
	int *dst = (int *) V_BASE(v);
#ifndef FORCE1PLANE
	int *m;
	int i, vplanes = V_USEDPLANES(v);
	long nwords;
#endif

	if (!V_LINEAR_P(v))
	  memmove (v->v.t.rowlist, rowoff, ((v->maxy+1) * sizeof(long)));
#ifdef FORCE1PLANE
	memset ((char *)dst, *V_BGMASK(v), V_SCRNSIZE(v));
#else
	if (*V_BG00(v))
	  zero((char *)dst, V_SCRNSIZE(v));
	else if (vplanes == 1)
	  memset ((char *)dst, *V_BGMASK(v), V_SCRNSIZE(v));
	else
	  {
	    /* do it the hard way */
	    for (nwords = V_SCRNSIZE(v) >> 1; nwords > 0; nwords -= vplanes)
	      {
		m = V_BGMASK(v);
		for (i = 0; i < vplanes; i++)
		  *dst++ = *m++;
	      }
	  }
#endif
}

/*
 * clrchars(v, x, y, n): clear n chars starting at position (x,y) on screen v
 */

#ifdef FORCE1PLANE
INLINE static
#endif
void
clrchars(v, x, y, n)
	SCREEN *v;
	int x, y, n;
{
	char *place;
	int *l, i;
#ifndef FORCE1PLANE
	int *m, j, vplanes;
#endif

	if (!x && n == v->maxx+1) {
		clrline(v, y);
		return;
	}
#ifndef FORCE1PLANE
	vplanes = V_USEDPLANES(v);
	vplanes += vplanes;
#endif

	if (y == v->cy && x == v->cx)
		place = v->cursaddr;
	else
		place = PLACE(v, x, y);

	l = V_BGMASK(v);
#ifndef FORCE1PLANE
	if (vplanes > 2) {
		if (x & 1) {
			char *p = place;
			for (j = v->cheight; j > 0; --j) {
				char *q = p;
				m = l;
				for (i = 0; i < vplanes; i += 2) {
					*q++ = *(char *) m++;
					++q;
				}
				p += v->planesiz;
			}
			place += v->planes;
			place += v->planes-1;
			--n;
		}
		if (n > 1) {
			int nbytes = n*v->planes;
			char *p = place;
			place += nbytes;

			if (*V_BG00(v)) {
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
					*p++ = *(char *) m++;
					++p;
				}
			}
		}
	} else
#endif
	{
		int nbytes = n*v->planes - v->planes + 1;

		for (i = v->cheight; i > 0; --i) {
			memset (place, *l, nbytes);
			place += v->planesiz;
		}
	}
}
