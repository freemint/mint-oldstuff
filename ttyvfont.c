/* a quick hack to load (old format) monospaced GDOS fonts, this can be
 * improved...
 *
 * if the font filename contains the string `iso' assume ISO 8859 latin-1
 * charset -> set input translation for non-ASCII keys (> 0x7f)
 * (actually it looks at char 0xa0 now... if space assumes latin-1.)
 *
 * sets the tty on stdin i.e. can be redirected like stty, run without
 * args to restore system font, run with --h for a small usage message
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <support.h>
#include <linea.h>
#include "vcon.h"

int ioctl ();
static unsigned char fontdata[256*16], rawbuf[256*16];

unsigned char iso2a[97] = {
#if 0
/* ISO Latin 1 to IBM code page 437 (classic IBM PC character set) */

   ' ', 173,155, 156, 'o', 157, '|',  21, '"', 'c',166,174,170,'-', 'R', '-',
   248, 241,253, '3',  39, 230,  20, 249, ',', '1',167,175,172,171, '?', 168,
   'A', 'A','A', 'A', 142, 143, 146, 128, 'E', 144,'E','E','I','I', 'I', 'I',
   'D', 165,'O', 'O', 'O', 'O', 153, 'x', 237, 'U','U','U',154,'Y', 'T', 225,
   133, 160,131, 'a', 132, 134, 145, 135, 138, 130,136,137,141,161, 140, 139,
   'd', 164,149, 162, 147, 'o', 148, 246, 237, 151,163,150,129,'y', 't', 152
#else
/* ISO Latin 1 to ST... */

   ' ', 173,155, 156, 'o', 157, '|',0xdd,0xb9,0xbd,166,174,170,'-',0xbe,0xff,
   248, 241,253,0xfe,0xba, 230,0xbc, 249, ',', '1',167,175,172,171, '?', 168,
   0xb6,'A','A',0xb7, 142, 143, 146, 128, 'E', 144,'E','E','I','I', 'I', 'I',
   'D', 165,'O', 'O', 'O',0xb8, 153, 'x',0xb2, 'U','U','U',154,'Y', 'T',0x9e,
   133, 160,131,0xb0, 132, 134, 145, 135, 138, 130,136,137,141,161, 140, 139,
   'd', 164,149, 162, 147,0xb1, 148, 246,0xb3, 151,163,150,129,'y', 't', 152
#endif
};

/* 0x80..0x9f (unused or same as 0..0x20 in ISO 8859):
 *   0    1   2    3    4    5    6    7    8    9   a   b   c   d    e    f

8*   €       ‚    ƒ    „    …    †    ‡    ˆ    ‰   Š   ‹   Œ       Ž    
9*       ‘   ’    “    ”    •    –    —    ˜    ™   š   ›   œ       ž    Ÿ
 */

/* 0xa0..0xff:
a*        ¡   ¢    £    ¤    ¥    ¦    §    ¨    ©   ª   «   ¬   ­    ®    ¯
b*   °    ±   ²    ³    ´    µ    ¶    ·    ¸    ¹   º   »   ¼   ½    ¾    ¿
c*   À    Á   Â    Ã    Ä    Å    Æ    Ç    È    É   Ê   Ë   Ì   Í    Î    Ï
d*   Ð    Ñ   Ò    Ó    Ô    Õ    Ö    ×    Ø    Ù   Ú   Û   Ü   Ý    Þ    ß
e*   à    á   â    ã    ä    å    æ    ç    è    é   ê   ë   ì   í    î    ï
f*   ð    ñ   ò    ó    ô    õ    ö    ÷    ø    ù   ú   û   ü   ý    þ    ÿ
 */

void usage(myname)
char *myname;
{
	if (myname) {
		char *s;
		if ((s = strrchr (myname, '/')))
			myname = s+1;
		if ((s = strrchr (myname, '\\')))
			myname = s+1;
	}
	if (!myname || !*myname)
		myname = "ttyvfont";
	fprintf(stderr, "usage: %s -rizgnv -f firstchar -c chars -o off -w rawfile font\n\n"
			"\t-r: file format raw bitmaps (otherwise GDOS unless file named .raw)\n"
			"\t-i: force latin-1 input translation (depends on char 0xa0 otherwise)\n"
			"\t-z: zap, empty char bitmaps (kept from previous font otherwise)\n"
			"\t-g: always read GDOS header\n"
			"\t-n: don't set font, read/convert only\n"
			"\t-v: show font search path, GDOS header contents, and some more talk\n\n"
			"\t-f 0-255 -c 1-256:\tfirst char, number of chars to get\n"
			"\t-o off:\t\t\tinput file offset\n"
			"\t-w rawfile:\t\toutput font file name (raw bitmap format)\n\n"
			"examples:\n"
			"\tttyvfont attp10cg\n"	
			"  (load Atari Typewriter 10 point lo-res)\n"
			"\tttyvfont; ttyvfont -f 0xb0 -c 48 -o 0xb00 ibm-16\n"
			"  (a system font with IBM line graphic chars at 0xb0-0xdf, assuming raw format)\n"
			"\tttyvfont -w aibm-16\n"
			"  (save as aibm-16.raw)\n"
			"\tttyvfont; ttyvfont -w system; ttyvfont iso-atari-16\n"
			"  (save system font, load latin-1...)\n"
			"\tttyvfont -r -f 0x81 -c 31 -o 16 -w funnyiso ./system.raw%s\n"
			"  (latin1 with ataris face chars, etc. at 0x81-0x9f...)%s\n", myname, (!ioctl (0, TCGETFONTBITS, fontdata) && !fontdata[0x79f] && fontdata[0x69f] == 0xe0) ? "\tœ" : "", (!ioctl (0, TCGETFONTBITS, fontdata) && !fontdata[0x79f] && fontdata[0x69f] == 0xe0) ? "\t\t\tžŸ" : "");
	exit (1);
}

int
main(int argc, char *argv[])
{
	static const char *const ext[] = { "fnt", "raw", (char *)0 };
	static __FONT fhdr;

	extern int optind,opterr;
	extern char *optarg;

	FILE *fp;
	char *f, *path = getenv("TTYVFONTPATH");
	const char *ttname = ttyname (0);
	int isop = 0, rawp = 0, zero = 0, usehdr = 0, verbose = 0, doset = 1;
	short tfontsiz[2] = {8, 16};
	long off = -1, firstc = -1, nchars = -1;
	char *outf = 0;
	int c;

	ioctl (0, TCGETFONTSIZE, &tfontsiz);

	opterr = 0;
	while( (c=getopt(argc,argv,"rizgnvo:f:c:w:"))!=EOF )
	{
		switch(c)
		{
			case 'r':
			rawp = 1;
			break;		

			case 'i':
			isop = 1;
			break;		

			case 'z':
			zero = 1;
			break;		

			case 'g':
			usehdr = 1;
			break;		

			case 'n':
			doset = 0;
			break;		

			case 'v':
			verbose = 1;
			break;		

			case 'o':
			off = strtoul (optarg, (char **)0, 0);
			break;		

			case 'f':
			firstc = strtoul (optarg, (char **)0, 0);
			break;		

			case 'c':
			nchars = strtoul (optarg, (char **)0, 0);
			break;		

			case 'w':
			outf = optarg;
			break;		

			case '?':
			usage(argv[0]);
			break;		
		}
	}

	if ((argc-optind > 1) || opterr) usage(argv[0]);

	if (!path)
		path = DEFFONTPATH;
	if (verbose) {
		printf ("font path %s\n", path);
		printf ("%s: chars %d bits wide, %d bits high\n",
			ttname, tfontsiz[0], tfontsiz[1]);
	}

	if (argc-optind < 1) {
		if (!outf) {			/* back to sysfont */
			if (!doset)
				return 0;
			if (verbose)
				printf ("%s: restoring system font\n", ttname);
			if (!ioctl (0, TCSETFONTBITS, (char *)0))
				ioctl (0, TCSETFONTXLAT, (char *)0);
			return 0;
		}
		if (ioctl (0, TCGETFONTBITS, fontdata)) {
			perror ("ioctl TCGETFONTSIZE");
			return 1;
		}
	} else {
		char *s;

		f = findfile (argv[optind], path, ext);
		for (s = f ? f : argv[optind]; *s; ++s) {
			if (*s == '\\') *s = '/';
		}

		if (!f || !(fp = fopen(f, "rb"))) {
			perror (f ? f : ((errno = ENOENT), argv[optind]));
			return 1;
		}
#if 0
		if (strstr (argv[optind], "iso"))
			isop = 1;
#endif
		if ((s = strrchr (f, '.')) && !stricmp (s, ".raw")) {
			rawp = 1;
		}

		if (!rawp && off < 0 && firstc < 0 && nchars < 0)
			usehdr = 1;
		if (usehdr) {
			if (verbose)
				printf ("\n%s: reading GDOS font header...\n", f);

			fread(&fhdr, sizeof fhdr, 1, fp);
			if (!(fhdr.flags & 4)) {	/* intel inside... */
				union w {short x; unsigned char b[2];} *p;
				union l {long x; unsigned char b[4];} *q
					= (union l *) &fhdr.dat_table;

				for (p = (union w *) &fhdr.font_id;
				     p < (union w *) &fhdr.name; ++p) {
					p->x = (p->b[1] << 8) | p->b[0];
				}
				for (p = (union w *) &fhdr.first_ade;
				     p < (union w *) &fhdr.h_table; ++p) {
					p->x = (p->b[1] << 8) | p->b[0];
				}
				q->x = (((((q->b[3] << 8) | q->b[2]) << 8) |
					q->b[1]) << 8) | q->b[0];
				for (p = (union w *) &fhdr.form_width;
				     p < (union w *) &fhdr.next_font; ++p) {
					p->x = (p->b[1] << 8) | p->b[0];
				}
			}
			firstc = fhdr.first_ade;
			nchars = fhdr.last_ade + 1 - firstc;
			off = (long) fhdr.dat_table;

			if (verbose) {
				printf (" 0x%04x %d point\t%s\n",
					fhdr.font_id, fhdr.size, fhdr.name);
				printf (" first char %d last char %d top %d ascent %d half %d descent %d bottom %d\n",
					fhdr.first_ade, fhdr.last_ade, fhdr.top, fhdr.ascent, fhdr.half, fhdr.descent, fhdr.bottom);
				printf (" max char width %d max cell width %d left offset %d right offset %d\n",
					fhdr.max_char_width, fhdr.max_cell_width, fhdr.left_offset, fhdr.right_offset);
				printf (" thicken %d underline size %d lighten 0x%04x skew 0x%04x flags 0x%04x\n",
					fhdr.thicken, fhdr.ul_size, fhdr.lighten, fhdr.skew, fhdr.flags);
			}
			if (!(fhdr.flags & 8)) {
				fprintf(stderr, "%s: not monospaced, needs conversion\n", f);
				return 1;
			}
			if (fhdr.form_width != nchars * ((tfontsiz[0]+7)>>3)) {
				fprintf(stderr, "%s: bad form_width %d\n", f, fhdr.form_width);
				return 1;
			}
		}

		if (firstc < 0) firstc = 0;
		if (nchars < 0 || firstc + nchars > 256) nchars = 256;
		if (off < 0) off = rawp ? 0 : 90 + nchars * 2;
		fseek(fp, off, SEEK_SET);         /* skip header */
		if (nchars < 256 && !zero)
			ioctl (0, TCGETFONTBITS, fontdata);
		if (rawp) {
			char *p = rawbuf, *q;
			int i, j, fw = tfontsiz[0] * 256 / 8;

			i = fread(rawbuf, 16, nchars, fp);
			if (verbose)
				printf ("\n%s: read %d raw char bitmaps\n", f, i);
			for (i = 0; i < nchars; ++i) {
				for (j = 0, q = fontdata+i+firstc; j < tfontsiz[1]; ++j) {
					*q = *p++;
					q += fw;
				}
			}
		} else if (nchars < 256) {
			char *p = rawbuf, *q;
			int i, fw = tfontsiz[0] * 256 / 8, rfw = tfontsiz[0] * nchars / 8;

			fread(rawbuf, 16, nchars, fp);
			for (i = 0, q = fontdata+firstc; i < tfontsiz[1]; ++i) {
				memcpy (q, p, rfw);
				p += rfw;
				q += fw;
			}
		} else {
			fread(fontdata, 16, 256, fp);
		}
		fclose(fp);
	}
	if (outf) {
		char *p = rawbuf, *q;
		int i, j, fw = tfontsiz[0] * 256 / 8;

		for (q = outf; *q; ++q) {
			if (*q == '\\') *q = '/';
		}
		if (!(q = strrchr (outf, '.')) || strchr (q, '/')) {
			
			if (!(q = malloc (strlen (outf) + sizeof ".raw"))) {
				perror (outf);
				return 1;
			}
			strcpy (q, outf);
			strcat (q, ".raw");
			outf = q;
		}
		for (i = 0; i < 256; ++i) {
			for (j = 0, q = fontdata+i; j < tfontsiz[1]; ++j) {
				*p++ = *q;
				q += fw;
			}
		}
		if (!(fp = fopen(outf, "wb")) ||
		    (i = fwrite(rawbuf, 16, 256, fp)) < 0 || fclose(fp)) {
			perror (outf);
			return 1;
		}
		if (verbose)
			printf ("\n%s: wrote %d raw char bitmaps\n", outf, i);
	}
	if (!doset)  return 0;
	if (ioctl (0, TCSETFONTBITS, fontdata)) {
		perror ("ioctl TCSETFONTBITS");
		return 1;
	} else {
		unsigned char buf[0x80];
		unsigned char *xlat = 0, *xlatr = 0, *xlatroff = 0;
		long xlatrsiz = 0;

		if (!isop) {
			char *q;
			int i, fw = tfontsiz[0] * 256 / 8;

			isop = 1;
			for (i = 0, q = fontdata+0xa0; i < tfontsiz[1]; ++i) {
				if (*q) {
		/* 0xa0 not a space, not iso */
					isop = 0;
					break;
				}
				q += fw;
			}
		}

		if (isop) {
			xlatr = iso2a;
			xlatroff = iso2a - ' ' - 0x80;
			xlatrsiz = sizeof iso2a;
		}
		if (xlatr) {
			unsigned char ch, *p, *q = buf;
			int i;

			for (i = 0x80; i < 0x100; ++i) {
				ch = 0;
				if ((p = memchr (xlatr, i, xlatrsiz)))
					ch = p - xlatroff;
				*q++ = ch;
			}
			xlat = buf;
		}

		if (ioctl (0, TCSETFONTXLAT, xlat)) {
			perror ("ioctl TCSETFONTXLAT");
			return 1;
		}
	}
	return 0;
}

