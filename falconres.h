/* data structure for VIDEL registers */

#define f030_col ((long *)		0xffff9800)
#define f030_xreg ((short*)		0xffff8282)
#define f030_yreg ((short*)		0xffff82a2)
#define f030_creg ((short*)		0xffff82c0)
#define f030_sreg ((short*)		0xffff8260)
#define f030_mreg ((short*)		0xffff820a)

#define VIDEL_DATA_FILENAME	"u:/etc/resolution"

#define VIDEL_DATA_IDENT	"VCF1"

typedef struct 
{
	char	ident[4];	/* VIDEL_DATA_IDENT */
	short	x_res;		/* width of screen in characters */
	short	y_res;		/* character lines per screen */
	short	nxreg[6];
	short	nyreg[6];
	short	nsreg[4];
	short	ncreg[2];
	short	nmreg[4];
} VIDEL_DATA;
