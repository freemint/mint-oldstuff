#include <stddef.h>
#include <sys/socket.h>
#include <sys/uio.h>

int
readv (s, iov, niov)
	int s;
	struct iovec *iov;
	size_t niov;
{
	struct msghdr m;

	m.msg_name = 0;
	m.msg_namelen = 0;
	m.msg_iov = iov;
	m.msg_iovlen = niov;
	m.msg_accrights = 0;
	m.msg_accrightslen = 0;

	return recvmsg (s, &m, 0);
}
