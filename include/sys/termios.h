#ifndef _TERMIOS_H
#define _TERMIOS_H

#define VEOF	0
#define VEOL	1
#define VERASE	2
#define VINTR	3
#define VKILL	4
#define VQUIT	5
#define VSUSP	6
#define VSTART	7
#define VSTOP	8
#define VMIN	9
#define VTIME	10
#define VLNEXT	11		/* <-- Not POSIX */
#define VWERASE	12		/* <-- Not POSIX */

#define NCCS	(VWERASE + 1)

typedef unsigned short tcflag_t;
typedef unsigned short speed_t;
typedef unsigned char cc_t;

struct termios {
  tcflag_t c_iflag;
  tcflag_t c_oflag;
  tcflag_t c_cflag;
  tcflag_t c_lflag;
  speed_t  _c_ispeed;
  speed_t  _c_ospeed;
  cc_t     c_cc[NCCS];
};

/* input flags */
#define BRKINT	0x0001
#define IGNBRK	0x0002
#define IGNPAR	0x0004
#define PARMRK	0x0008
#define INPCK	0x0010
#define ISTRIP	0x0020
#define INLCR	0x0040
#define IGNCR	0x0080
#define ICRNL	0x0100
#define IXON	0x0200
#define IXOFF	0x0400

/* output modes */
#define OPOST	0x0001

/* control modes */
#define CLOCAL	0x0001
#define CREAD	0x0002
#define CS5	0x0004
#define CS6	0x0008
#define CS7	0x0010
#define CS8	0x0020
# define CSIZE	(CS5 | CS6 | CS7 | CS8)
#define CSTOPB	0x0040
#define HUPCL	0x0080
#define PARENB	0x0100
#define PARODD	0x0200

/* local modes */
#define ECHOE	0x0001
#define ECHOK	0x0002
#ifndef _IOCTL_H
#define ECHO	0x0004
#endif /* _IOCTL_H */
#define ECHONL	0x0008
#define ICANON	0x0010
#define ISIG	0x0020
#ifndef _IOCTL_H
#define NOFLSH	0x0040
#endif /* _IOCTL_H */
#define IEXTEN	0x0080
#ifndef _IOCTL_H
#define TOSTOP	0x0100
#endif /* _IOCTL_H */

/* actions for tcflow() */
#define TCOOFF		0
#define TCOON		1
#define TCIOFF		2
#define TCION		3

/* actions for tcflush() */
#define TCIFLUSH	0
#define TCOFLUSH	1
#define TCIOFLUSH	3

/* actions for tcsetattr() */
#define TCSANOW		0
#define TCSADRAIN	1
#define TCSAFLUSH	2

/* internal use only */
#define _TF_STOPBITS	0x0003
#define _TF_1STOP	0x0001
#define _TF_15STOP	0x0002
#define	_TF_2STOP	0x0003
#define _TF_CHARBITS	0x000C
#define _TF_8BIT	0
#define _TF_7BIT	0x4
#define _TF_6BIT	0x8
#define _TF_5BIT	0xC

/* baud rate codes */
#ifndef _IOCTL_H
#define B0		0
#define B50		1
#define B75		2
#define B110		3
#define B135		4
#define B150		5
#define B200		6
#define B300		7
#define B600		8
#define B1200		9
#define B1800		10
#define B2400		11
#define B4800		12
#define B9600		13
#define B19200		14
#define B38400		15
#endif /* _IOCTL_H */

#define CERASE	'\177'
#define CKILL	'\025'
#define CINTR	'\003'
#define CQUIT	'\025'
#define CSTART	'\021'
#define CSTOP	'\023'
#define CEOF	'\004'
#define CBRK	'\000'
#define CSUSP	'\032'
#define CDSUSP	'\031'
#define CRPRNT	'\022'
#define CFLUSH	'\017'
#define CWERASE	'\027'
#define CLNEXT	'\026'

#define OTTYDISC	0
#define TIOCHPCL	0

#define CR1	0
#define CR2	0
#define CR3	0

#define NL1	0
#define NL2	0
#define NL3	0

#define BS1	0
#define FF1	0
#define TAB1	0
#define TAB2	0
#define EXTA	0
#define EXTB	0

#define B134	B135

#ifdef __STDC__
__EXTERN speed_t cfgetispeed	__PROTO((const struct termios *stp));
__EXTERN speed_t cfgetospeed	__PROTO((const struct termios *stp));
__EXTERN int	cfsetispeed	__PROTO((struct termios *stp,
						speed_t baudcode));
__EXTERN int	cfsetospeed	__PROTO((struct termios *stp,
						speed_t baudcode));
__EXTERN int	tcdrain		__PROTO((int fd));
__EXTERN int	tcflow		__PROTO((int fd, int action));
__EXTERN int	tcflush		__PROTO((int fd, int action));
__EXTERN int	tcgetattr	__PROTO((int fd, struct termios *stp));
__EXTERN int	tcsendbreak	__PROTO((int fd, int duration));
__EXTERN int	tcsetattr	__PROTO((int fd, int action, 
						const struct termios *stp));
#else
extern unsigned short cfgetispeed();
extern unsigned short cfgetospeed();
extern int cfsetispeed();
extern int cfsetospeed();
extern int tcdrain();
extern int tcflow();
extern int tcflush();
extern int tcgetattr();
extern int tcsendbreak();
extern int tcsetattr();
#endif

#endif /* _TERMIOS_H */
