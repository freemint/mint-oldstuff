#	@(#)Makefile	5.12 (Berkeley) 1/21/91
CPPFLAGS=-I../../include -DMINT
CFLAGS=-traditional $(ADDITIONALCFLAGS)

PROG=	getty
SRCS=	main.c init.c subr.c gettytab.c ttydefaults.c
OBJS=	main.o init.o subr.o gettytab.o ttydefaults.o
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN5=	gettytab.0 ttys.0
MAN8=	getty.0

$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(OBJS) ../../usretc/syslog/libsyslog.a -liio
	../../bin/ttramify $(PROG)

clean:
	/bin/rm $(PROG) *.o

install: $(PROG)
	-mv $(USRSBINDIR)/$(PROG)$(PRGEXT) $(USRSBINDIR)/$(PROG).old
	cp $(PROG) $(USRSBINDIR)/$(PROG)$(PRGEXT)
	chmod 0755 $(USRSBINDIR)/$(PROG)$(PRGEXT)
