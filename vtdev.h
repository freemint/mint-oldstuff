/* kernel calls... */
#define TGETDATE	(*kernel->dos_tab[0x2a])
#define TGETTIME	(*kernel->dos_tab[0x2c])
#define FOPEN	(*kernel->dos_tab[0x3d])
#define FCLOSE	(*kernel->dos_tab[0x3e])
#define FREAD	(*kernel->dos_tab[0x3f])
#define FSEEK	(*kernel->dos_tab[0x42])
#define MXALLOC	(*kernel->dos_tab[0x44])
#define FDATIME	(*kernel->dos_tab[0x44])
#define MALLOC	(*kernel->dos_tab[0x48])
#define FCNTL	(*kernel->dos_tab[0x104])
#define FINSTAT	(*kernel->dos_tab[0x105])
#define FGETCHAR (*kernel->dos_tab[0x107])
#define PGETPID	(*kernel->dos_tab[0x10b])

#define SPRINTF	(*kernel->sprintf)
#define DEBUG	(*kernel->debug)
#define ALERT	(*kernel->alert)
#define TRACE	(*kernel->trace)
#define FATAL	(*kernel->fatal)
#define KMALLOC (*kernel->kmalloc)
#define KFREE	(*kernel->kfree)
#define NAP	(*kernel->nap)
#define SLEEP	(*kernel->sleep)
#define WAKESELECT (*kernel->wakeselect)

#include <string.h>
/* should be equivalent... atleast for mintlib. */
#define quickmove memmove
#define zero bzero
/* only possible because main open()s /dev/ttyv0 that calls this. */
#define kcore(x) MXALLOC((long)(x),0)
/* should this do something? */
#define checkkeys() (0)
#define kmalloc KMALLOC
#define kfree KFREE
#define bconout Bconout
#define yield() SLEEP(1, 0l)

/* number of terminals (max) */
#define N_VT  10

#define MAX_PLANES 8

#define ALT_1 0x780000L
#define ALT_2 0x790000L
#define ALT_0 0x810000L

#ifdef __GNUC__
#define INLINE inline
#define ITYPE long	/* gcc's optimizer likes 32 bit integers */
#else
#define INLINE
#define ITYPE int
#endif

struct screen;

typedef void (*Vfunc) P_((struct screen *, int));

typedef struct screen {
	short	hidecnt;	/* cursor hide count */
	short	mcurx, mcury;	/* current mouse X, Y position */
	char	mdraw;
	char	mouseflag;
	long	junk1;
	short	savex, savey;	/* saved X, Y position */
	short	msavelen;	/* mouse save stuff */
	long	msaveaddr;
	short	msavestat;
	union {
		long	msavearea[64];
/* additional stuff, NOT for ttyv0! */
		struct {
			Vfunc	state;
			short	vescy1;
			short	cshape;
			short	usedplanes;
			int	fgmask[MAX_PLANES], fgff;
			int	bgmask[MAX_PLANES], bg00;
			char	*vbase;		/* screens address */
			char	*rowlist;	/* pointer to line offsets */
			short	on;		/* 0 if screen `stored'... */
#define V_USED	1	/* not stored, open */
#define V_FREE	2	/* not stored, closed, free screen memory later */
		} t;
	} v;
	long	user_tim, next_tim; /* time vector stuff */
	long	user_but, user_cur,
		user_mot;	/* more user vectors */
	short	cheight;	/* character height */
	short	maxx;		/* number of characters across - 1 */
	short	maxy;		/* number of characters high - 1 */
	short	linelen;	/* length (in bytes) of a line of characters */	
	short	bgcol;		/* background color */
	short	fgcol;		/* foreground color */
	char	*cursaddr;	/* cursor address */
	short	v_cur_of;	/* ??? */
	short	cx, cy;		/* current (x,y) position of cursor */
	char	period;		/* cursor flash period (in frames) */	
	char	curstimer;	/* cursor flash timer */
	char	*fontdata;	/* pointer to font data */
	short	firstcode;	/* first ASCII code in font */
	short	lastcode;	/* last ASCII code in font */
	short	form_width;	/* # bytes/scanline in font data */
	short	xpixel;
	char	*fontoff;	/* pointer to font offset table */
	char	flags;		/* e.g. cursor on/off */
	char	reserved;
	short	ypixel;
	short	width;		/* length of a screen scan line */
	short	planes;		/* number of planes on screen */
	short	planesiz;	/* length of a screen scan line */
} SCREEN;

/* possible flags for cursor state, etc. */
#define CURS_FLASH	0x01		/* cursor flashing */
#define CURS_FSTATE	0x02		/* cursor in flash state */
#define CURS_ON		0x04		/* cursor on */
#define FWRAP		0x08		/* wrap cursor at end of line */
#define FINVERSE	0x10		/* invert text */
#define FUNDERLINE	0x20		/* EXTENSION: underline text */
#define CURS_UPD	0x40		/* cursor update flag */
#define FBOLD		0x80		/* EXTENSION: boldface */

/* possible flags for cursor shape */
#define CURS_STEADY	0x01		/* flashing off */
#define CURS_UNDERLINE	0x00		/* underline cursor */
#define CURS_BLOCK	0x02		/* block cursor */

#define base (*((char **)0x44eL))

#ifdef FORCE1PLANE
#define VT00XCON
#endif

#ifdef VT00XCON
#define xconout	((long *) 0x57eL)

#define V_BASE(x) ((x)->v.t.vbase)
#define V_LINEOFF(x, lx4) (*(long *)(rowoff+(lx4)))
#define V_LINE(x, lx4) ((x)->v.t.vbase + *(long *)((x)->v.t.rowlist+(lx4)))
#define V_LINEAR_P(x) ((x)->v.t.on)
#define V_ESCY1(x) (&(x)->v.t.vescy1)
#define V_STATE(x) (&(x)->v.t.state)
#define V_FGMASK(x) ((x)->v.t.fgmask)
#define V_BGMASK(x) ((x)->v.t.bgmask)
#define V_CSHAPE(x) ((x)->v.t.cshape)
#ifndef FORCE1PLANE
#define V_USEDPLANES(x) ((x)->v.t.usedplanes)
#define V_FGFF(x) (&(x)->v.t.fgff)
#define V_BG00(x) (&(x)->v.t.bg00)
#endif
#define V_SCRNSIZE(x) (scrnsize)
#else
#define escy1 (*((short *)0x4acL))

#define V_BASE(x) ((x) == v00 ? base : (x)->v.t.vbase)
#define V_LINEOFF(x, lx4) ((((long *)rowoff)[1] == (x)->linelen ? \
				*(long *)(rowoff+(lx4)) : \
				((lx4)>>2)*(long)(x)->linelen))
#define V_LINE(x, lx4) ((x) == v00 ? (base + V_LINEOFF((x), (lx4))) : \
			((x)->v.t.vbase + *(long *)((x)->v.t.rowlist+(lx4))))
#define V_LINEAR_P(x) ((x) == v00 || (x)->v.t.on)
#define V_ESCY1(x) ((x) == v00 ? &escy1 : &(x)->v.t.vescy1)
#define V_STATE(x) ((x) == v00 ? &v00state : &(x)->v.t.state)
#define V_FGMASK(x) ((x) == v00 ? fgmask : (x)->v.t.fgmask)
#define V_BGMASK(x) ((x) == v00 ? bgmask : (x)->v.t.bgmask)
#define V_CSHAPE(x) ((x) == v00 ? CURS_BLOCK : (x)->v.t.cshape)
#define V_USEDPLANES(x) ((x) == v00 ? (x)->planes : (x)->v.t.usedplanes)
#define V_FGFF(x) ((x) == v00 ? &fgff : &(x)->v.t.fgff)
#define V_BG00(x) ((x) == v00 ? &bg00 : &(x)->v.t.bg00)
#define V_SCRNSIZE(x) ((x) == v00 ? ((x)->maxy+1)*(long)(x)->linelen : scrnsize)
#endif

#define V_NFONTBYTES(x) ((long) (x)->form_width * (x)->cheight)

extern struct ttyv {
	SCREEN	*v;
	struct tty tt;
	char	*loadfontdata;	/* (allocated) storage for loaded fonts */
	long	loadfontbytes;	/* size of ... */
	unsigned char readxlat[0x80];	/* keys > 0x7f translation */
} ttys[];
extern DEVDRV vcon_device;
extern struct dev_descr devinfo[];
extern int vcurrent;
extern long pgrp;
extern short hardscroll, leaving, os_version;
extern SCREEN *v00, v0x[], *current;
extern char *chartab[256*2];
extern long scrnsize;
extern char *rowoff;
#ifndef FORCE1PLANE
extern void (*vpaint) P_((SCREEN *, int, char *));
#endif
#ifndef VT00XCON
extern int fgmask[MAX_PLANES], bgmask[MAX_PLANES], fgff, bg00;
#endif

#ifdef VMODE
SCREEN *getvtmode P_((SCREEN *v00));
void showscreen P_((int vt, SCREEN *v, char *vbase, int save));
#else
#define getvtmode(v00) (v00)
#define showscreen(vt, v, vbase, save) Setscreen (-1l, vbase, -1l)
#endif

void setup_chartab P_((SCREEN *));
#ifndef FORCE1PLANE
void flash P_((SCREEN *));
void paint P_((SCREEN *, int, char *)),
	 paint8c P_((SCREEN *, int, char *)),
	 paint816m P_((SCREEN *, int, char *));
void clear P_((SCREEN *));
void clrline P_((SCREEN *, int));
void clrchars P_((SCREEN *, int, int, int));
#endif

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
			place += V_SCRNSIZE(v) - v->linelen;
		else if (y) {
			y+=y;	/* Make Y into index for longword array. */
			y+=y;	/* Two word-size adds are faster than a 2-bit shift. */
			place += V_LINEOFF(v, y);
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
