/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)error.c	5.1 (Berkeley) 3/7/91";
#endif /* not lint */

/*
 * Errors and exceptions.
 */

#include "shell.h"
#include "main.h"
#include "options.h"
#include "output.h"
#include "error.h"
#include <signal.h>
#ifdef __STDC__
#include "stdarg.h"
#else
#include <varargs.h>	
#endif
#include <errno.h>


/*
 * Code to handle exceptions in C.
 */

struct jmploc *handler;
int exception;
volatile int suppressint;
volatile int intpending;
char *commandname;


/*
 * Called to raise an exception.  Since C doesn't include exceptions, we
 * just do a longjmp to the exception handler.  The type of exception is
 * stored in the global variable "exception".
 */

void
exraise(e) {
	if (handler == NULL)
		abort();
	exception = e;
	longjmp(handler->loc, 1);
}


/*
 * Called from trap.c when a SIGINT is received.  (If the user specifies
 * that SIGINT is to be trapped or ignored using the trap builtin, then
 * this routine is not called.)  Suppressint is nonzero when interrupts
 * are held using the INTOFF macro.  The call to _exit is necessary because
 * there is a short period after a fork before the signal handlers are
 * set to the appropriate value for the child.  (The test for iflag is
 * just defensive programming.)
 */

void
onint() {
	if (suppressint) {
		intpending++;
		return;
	}
	intpending = 0;
#ifdef BSD
	sigsetmask(0);
#endif
	if (rootshell && iflag)
		exraise(EXINT);
	else
		_exit(128 + SIGINT);
}



void
error2(a, b)
	char *a, *b;
	{
	error("%s: %s", a, b);
}


/*
 * Error is called to raise the error exception.  If the first argument
 * is not NULL then error prints an error message using printf style
 * formatting.  It then raises the error exception.
 */

#ifdef __STDC__
void
error(char *msg, ...) {
#else
void
error(va_alist)
	va_dcl
	{
	char *msg;
#endif
	va_list ap;

	CLEAR_PENDING_INT;
	INTOFF;
#ifdef __STDC__
	va_start(ap, msg);
#else
	va_start(ap);
	msg = va_arg(ap, char *);
#endif
#ifdef DEBUG
	if (msg)
		TRACE(("error(\"%s\") pid=%d\n", msg, getpid()));
	else
		TRACE(("error(NULL) pid=%d\n", getpid()));
#endif
	if (msg) {
		if (commandname)
			outfmt(&errout, "%s: ", commandname);
		doformat(&errout, msg, ap);
		out2c('\n');
	}
	va_end(ap);
	flushall();
	exraise(EXERROR);
}



/*
 * Table of error messages.
 */

struct errname {
	short errcode;		/* error number */
	short action;		/* operation which encountered the error */
	char *msg;		/* text describing the error */
};


#define ALL (E_OPEN|E_CREAT|E_EXEC)

STATIC const struct errname errormsg[] = {
	EERROR, ALL,	"generic error",
	EDRVNR, ALL,	"drive not ready",
	EUNCMD, ALL,	"unknown drive command",
	E_CRC, ALL,	"CRC error",
	EBADRQ, ALL,	"bad drive request",
	E_SEEK, ALL,	"seek error",
	EMEDIA, ALL,	"unknown medium format",
	ESECNF, ALL,	"sector not found",
	EPAPER, ALL,	"out of paper",
	EWRITEF, ALL,	"write fault",
	EREADF, ALL,	"read fault",
	EROFS, E_CREAT,	"read-only filesystem",
	E_CHNG, ALL,	"media changed",
	EUNDEV, ALL,	"unknown drive",
	EBADSF, ALL,	"bad sector on format",
	EOTHER, ALL,	"insert other disk request",
	EINVAL, ALL,	"unimplemented system call",
	ENOENT, ALL,	"no such file or directory (or pid)",
	EPATH, ALL,	"path not found",
	EMFILE, E_OPEN|E_CREAT,	"no more open files",
	EACCESS, ALL,	"access denied",
	EBADF, ALL,	"bad file descriptor",
	ENOMEM, ALL,	"out of memory",
	EFAULT, ALL,	"invalid memory block address",
	ENXIO, ALL,	"invalid drive specification",
	EXDEV, ALL,	"cannot move across file systems",
	EBADARG, ALL,	"bad arguments to system call",
	EINTRN, ALL,	"internal OS error",
	EPLFMT, E_EXEC,	"bad program load format",
	EGSBF, ALL,	"memory block growth failure",
	EMLINK, ALL,	"too many symbolic links",
	EEXIST, E_CREAT,	"file exists",
	ENAMETOOLONG, ALL, "file name too long",
	ERANGE, ALL,	"math range error",
	EDOM, ALL,	"math domain error"
};


/*
 * Return a string describing an error.  The returned string may be a
 * pointer to a static buffer that will be overwritten on the next call.
 * Action describes the operation that got the error.
 */

char *
errmsg(e, action) {
	struct errname const *ep;
	static char buf[12];

	for (ep = errormsg ; ep->errcode ; ep++) {
		if (ep->errcode == e && (ep->action & action) != 0)
			return ep->msg;
	}
	fmtstr(buf, sizeof buf, "error %d", e);
	return buf;
}
