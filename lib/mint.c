/*
 *	int __mint_read  (int, void *, unsigned int);
 *	int __mint_write (int, void *, unsigned int);
 *
 *	Q&D posix like read() and write(). They (should) return
 *	EWOULDBLOCK for nonblocking mode and no data avail / no
 *	space left. 
 *
 *	int __fork_and_exit();
 *
 *	This acts like 'if (fork()) exit();' with a nonblocking fork.
 *	But unfortunately MiNT's fork() does block the parent until
 *	child does exec() or exit(). So use fork_and_exit() as a temp.
 *	workaround.
 *
 *	12/28/93, kay roemer <roemer@rbi.informatik.uni-frankfurt.de>
 */

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ioctl.h>
#include <osbind.h>
#include <mintbind.h>
#include <setjmp.h>
#include <signal.h>

#define UNLIMITED	1000000

int
__check_nbio_read (int fd)
{
	unsigned long buf;
	int s;

	if (Fcntl (fd, 0, F_GETFL) & O_NDELAY) {
		s = Fcntl (fd, &buf, FIONREAD);
		if (!s && buf < UNLIMITED) {
			errno = EWOULDBLOCK;
			return -1;
		}
	}
	return 0;
}

int
__check_nbio_write (int fd)
{
	unsigned long buf;
	int s;

	if (Fcntl (fd, 0, F_GETFL) & O_NDELAY) {
		s = Fcntl (fd, &buf, FIONWRITE);
		if (!s && buf < UNLIMITED) {
			errno = EWOULDBLOCK;
			return -1;
		}
	}
	return 0;
}

int
__mint_read (int fd, void *buffer, unsigned int buflen)
{
	int r, s;
	unsigned long buf;
	extern int errno;

	r = Fread (fd, buflen, buffer);
	if (r <= 0) {
		if (r < 0) {
			errno = -r;
			return -1;
		}
		if (buflen && Fcntl (fd, 0, F_GETFL) & O_NDELAY) {
			s = Fcntl (fd, &buf, FIONREAD);
			if (!s && buf < UNLIMITED) {
				errno = EWOULDBLOCK;
				return -1;
			}
		}
	}
	return r;
}

int
__mint_write (int fd, void *buffer, unsigned int buflen)
{
	int r, s;
	unsigned long buf;
	extern int errno;

	r = Fwrite (fd, buflen, buffer);
	if (r <= 0) {
		if (r < 0) {
			errno = -r;
			return -1;
		}
		if (buflen && Fcntl (fd, 0, F_GETFL) & O_NDELAY) {
			s = Fcntl (fd, &buf, FIONWRITE);
			if (!s && buf < UNLIMITED) {
				errno = EWOULDBLOCK;
				return -1;
			}
		}
	}
	return r;
}

static int fd_flags[32];
static jmp_buf jmpbuf;
static struct sigaction sas[32];
static long sigmask;

static int
child_proc (long pip)
{
	char buf;
	int i;
	
	if (read (pip, &buf, 1)) exit (0);
	close (pip);
	for (i = 0; i < 32; ++i) {
		if (fd_flags[i] >= 0)
			fcntl (i, F_SETFD, fd_flags[i]);
	}
	for (i = 0; i < 32; ++i) {
		sigaction (i, &sas[i], 0);
	}
	longjmp (jmpbuf, 1);
}
	
int
__fork_and_exit (void)
{
	static int i, pip[2];
	extern int tfork (int (*)(long), long);

	if (pipe (pip) < 0) return -1;
	sigmask = sigsetmask (~0L);
	for (i = 0; i < 32; ++i) {
		fd_flags[i] = fcntl (i, F_GETFD, 0);
		fcntl (i, F_SETFD, i == pip[1] ? FD_CLOEXEC : 0);
	}
	for (i = 0; i < 32; ++i) {
		sigaction (i, 0, &sas[i]);
	}
	if (!setjmp (jmpbuf)) {
		tfork (child_proc, pip[0]);
		exit (0);
	}
	sigsetmask (sigmask);
	return 0;
}
