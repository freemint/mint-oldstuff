/*
 * Ttyent.h	A header file for getttyent().
 * Version 1.0 by S.R.Usher 1991.
 *
 * Placed into the public domain on 10th September, 1994.
 *
 */
#ifndef _TTYENT_H_
#define _TTYENT_H_

struct ttyent {
	char *ty_name;		/* Terminal device name */
	char *ty_getty;		/* Command to execute, usually getty */
	char *ty_type;		/* Terminal type */
	int   ty_status;	/* Status flags */
	char *ty_window;	/* Windowing system startup command */
	char *ty_comment;	/* Anything which appears as a comment */
};

#define TTY_ON		0x01	/* Enable logins on this tty */
#define TTY_SECURE	0x02	/* Allow root to login on this tty */

struct ttyent *getttyent();
struct ttyent *getttynam();

#endif /* _TTYENT_H_ */
