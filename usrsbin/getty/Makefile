#	@(#)Makefile	5.12 (Berkeley) 1/21/91

include ../../.builddir
include ../../MakeVars

CPPFLAGS=-I$(BUILDDIR)/include -DMINT
CFLAGS=-traditional $(ADDITIONALCFLAGS)

PROG=	getty
SRCS=	main.c init.c subr.c gettytab.c ttydefaults.c
OBJS=	main.o init.o subr.o gettytab.o ttydefaults.o
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN5=	gettytab.0 ttys.0
MAN8=	getty.0

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(BUILDDIR)/usretc/syslog/libsyslog.a
	$(FASTRAMPRG) $(PROG)

clean:
	$(RM) $(PROG) *.o

install: $(PROG)
	-$(MV) $(USRSBINDIR)/$(PROG)$(PRGEXT) $(USRSBINDIR)/$(PROG).old
	$(CP) $(PROG) $(USRSBINDIR)/$(PROG)$(PRGEXT)
	$(CHMOD) 0755 $(USRSBINDIR)/$(PROG)$(PRGEXT)
