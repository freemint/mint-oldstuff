/*
 *	WARNING: readv() and writev() work only with sockets! On other files
 *	you will get EINVAL.
 */

#ifndef _SYS_UIO_H
#define _SYS_UIO_H

#ifndef _COMPILER_H
#include <compiler.h>
#endif

#include <sys/socket.h>

__EXTERN int	readv	__PROTO((int, struct iovec *, _SIZE_T));
__EXTERN int	writev	__PROTO((int, struct iovec *, _SIZE_T));

#endif
