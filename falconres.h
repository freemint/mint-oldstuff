/* data structure for VIDEL registers */

/* Your attention is drawn to the fact that this is
   the falconres.h for vconsd v0.9b - don't mix them
   up with the old versions! - smoore */

#define f030_col ((long *)		0xffff9800)

#define VIDEL_hw_xregs      ((short *)	0xffff8282)
#define VIDEL_hw_yregs      ((short *)	0xffff82A2)
#define VIDEL_hw_vco        ((short *)	0xffff82C0)
#define VIDEL_hw_c_s        ((short *)	0xffff82C2)
#define VIDEL_hw_offsets    ((short *)	0xffff820E)
#define VIDEL_hw_sync       ((short *)	0xffff820A)
#define VIDEL_hw_p_o        ((unsigned char *)	0xffff8256)
#define VIDEL_hw_spshift    ((short *)	0xffff8266)
#define VIDEL_hw_stshift    ((short *)	0xffff8260)

#define VIDEL_DATA_FILENAME	"u:/etc/resolution"

/* Yep, the structure's changed. */

#define VIDEL_DATA_IDENT	"VCF3"

typedef struct 
{
	char	ident[4];	/* VIDEL_DATA_IDENT */
	short	x_res;		/* width of screen in characters */
	short	y_res;		/* character lines per screen */
	short	nxreg[6]; /* horizontal registers */
	short	nyreg[6]; /* vertical registers */
	short	nvco;			/* video clock oscillators */
	short	nc_s;			/* scan flags */
	short	noff[2];	/* line offsets */
	short	nsync;		/* synchronisation */
	unsigned char	np_o;	/* horizontal hscroll shift */
	unsigned char	st_flag; /* st-compatability flag */
	short	nsps;	/* spshift */
	short	nsts;	/* stshift */
} VIDEL_DATA;
