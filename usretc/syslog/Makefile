#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such redistribution and
# use acknowledge that the software was developed by the University
# of California, Berkeley.  The name of the University may not be
# used to endorse or promote products derived from this software
# without specific prior written permission.  THIS SOFTWARE IS PROVIDED
# ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
# WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND
# FITNESS FOR A PARTICULAR PURPOSE.
#
# @(#)Makefile	5.1 (Berkeley) 9/27/88
#

include ../../.builddir
include ../../MakeVars

#
# compile flags for daemon and library, syslogd needs one or more of
# -DSYSLOG_KERNEL, -DSYSLOG_UNIXAF and -DSYSLOG_INET
#
# syslog library needs either -DSYSLOG_INET  or -DSYSLOG_UNIXAF

#SYSLOGD_FLAGS = -DSYSLOG_KERNEL -DSYSLOG_UNIXAF
#SYSLOG_FLAGS = -DSYSLOG_UNIXAF

#SYSLOGD_FLAGS = -DSYSLOG_INET
#SYSLOG_FLAGS = -DSYSLOG_INET

CFLAGS= $(ADDITIONALCFLAGS) -DMINT -I$(BUILDDIR)/include -I.

LIBS	=	libsyslog.a	
SRCS	=	syslogd.c
SYSLOGD_OBJS=	syslogd.o $(BUILDDIR)/lib/libusers.a
OBJS	=	${SYSLOGD_OBJS} syslog_test.o sendlog.o logger.o \
		syslog.o readlog.o 
MAN	=	syslogd.0
#LDFLAGS = -lsocket
LDFLAGS = $(BUILDDIR)/lib/libusers.a
#TARGETS =	syslogd syslog_test sendlog logger readlog libsyslog.a
# logger won't build with the current mintlibs,
#	readlog won't ever!
#TARGETS =	syslogd syslog_test sendlog libsyslog.a
TARGETS =	libsyslog.a syslogd syslog_test sendlog logger

all:	${TARGETS}	

/usr/lib/libresolv.a:
	@$(ECHO) "You need to build libresolv.a first"
	@false

syslog.o: syslog.c
	${CC} -c  ${CFLAGS} ${SYSLOG_FLAGS} syslog.c

libsyslog.a:	syslog.o
	$(AR) r libsyslog.a syslog.o

syslogd.o: syslogd.c
	${CC} -traditional -c  ${CFLAGS} ${SYSLOGD_FLAGS} syslogd.c

readlog: readlog.o syslog.o
	${CC}  -o $@ ${CFLAGS} readlog.o syslog.o  ${LDFLAGS}
	$(FASTRAMPRG) $@

syslogd: $(SYSLOGD_OBJS)
	${CC} -o $@ ${CFLAGS} $(SYSLOGD_OBJS) ${LDFLAGS}
	$(FASTRAMPRG) $@

syslog_test: syslog_test.o syslog.o ${LIBS}
	${CC} ${CFLAGS} -o syslog_test syslog_test.o syslog.o ${LDFLAGS}
	$(FASTRAMPRG) $@

sendlog: sendlog.o syslog.o ${LIBS}
	${CC}  ${CFLAGS} -o sendlog sendlog.o syslog.o ${LDFLAGS}
	$(FASTRAMPRG) $@

logger: logger.o syslog.o ${LIBS}
	${CC}  ${CFLAGS} -o logger logger.o syslog.o ${LDFLAGS}
	$(FASTRAMPRG) $@

clean:
	$(RM) -f ${OBJS} core ${TARGETS}

cleandir: clean
	$(RM) -f ${MAN} tags .depend

depend: ${SRCS}
	mkdep -p ${CFLAGS} ${SRCS}

install: syslogd # ${MAN}
	$(CP) syslogd $(USRETCDIR)/syslogd$(TTPEXT)
	$(CHMOD) 0755 $(USRETCDIR)/syslogd$(TTPEXT)
	$(CP) sendlog $(USRBINDIR)/sendlog$(TTPEXT)
	$(CHMOD) 0755 $(USRBINDIR)/sendlog$(TTPEXT)
	$(CP) logger $(USRBINDIR)/logger$(TTPEXT)
	$(CHMOD) 0755 $(USRBINDIR)/logger$(TTPEXT)
#	install -s -o bin -g bin -m 755 syslogd ${DESTDIR}/etc/syslogd
# 	install -c -o bin -g bin -m 444 syslogd.0 ${DESTDIR}/usr/man/cat8

lint: ${SRCS}
	lint ${CFLAGS} ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
