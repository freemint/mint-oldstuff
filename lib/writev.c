/*
 * Writev.c - An almost bug-for-bug compatible version (according to the SunOS
 *             4.1.1 write/writev(2V) manual page).
 *
 * 1.0	S.R.Usher	2nd August 1994. (02/08/94)
 *
 * Placed into the public domain 10th September, 1994.
 *
 */

#include <stdio.h>
#include <sys/errno.h>

extern void *malloc();

struct  iovec {
	char	*iov_base;
	int     iov_len;
};


int writev(fd, iov, iovcnt)
int fd;
struct iovec *iov;
int iovcnt;
{
	register int i;
	char *buffer, *ptr;
	long length = 0;
 	int retval;
	
	if ((iovcnt <= 0) || (iovcnt > 16))
	{
		errno = EINVAL;
		return -1;
	}

	for (i = 0; i < iovcnt; i++)
	{
#ifdef DEBUG
		fprintf(stderr, "writev: iov[%d].iov_len = %d\n", i, iov[i].iov_len);
#endif
		length += iov[i].iov_len;
	}

#ifdef DEBUG
	fprintf(stderr, "writev: length = %ld\n", length);
#endif

	if (length < 0)
	{
		errno = EINVAL;
		return -1;
	}
	
	if ((buffer = (char *)malloc(length)) == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	for (i = 0, ptr = buffer; i < iovcnt; i++)
	{
#ifdef DEBUG
		fprintf(stderr, "writev: iov[%d].iov_base = '", i);
		fwrite(iov[i].iov_base, 1, iov[i].iov_len, stderr);
		fprintf(stderr, "'\n");
#endif
		bcopy(iov[i].iov_base, ptr, iov[i].iov_len);
		ptr += (long)iov[i].iov_len;
	}

#ifdef DEBUG
	fprintf(stderr, "writev: complete buffer = '");
	fwrite(buffer, 1, length, stderr);
	fprintf(stderr, "'\n");
#endif

	retval = write(fd, buffer, length);

	free(buffer);

	return retval;
}
