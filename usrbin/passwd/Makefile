# Makefile for passwd

include ../../.builddir
include ../../MakeVars

CPPFLAGS = -I$(BUILDDIR)/include -DMINT -DLOGIN -DBZERO -DSHELL=\"$(DEFAULTSHELL)\" -DOWN_TIMEOUT
CFLAGS = -g $(ADDITIONALCFLAGS)

EXTRALIBS = $(BUILDDIR)/lib/libusers.a $(BUILDDIR)/usretc/syslog/libsyslog.a

PROGRAMS=passwd

all: $(PROGRAMS)

passwd: passwd.o $(EXTRALIBS)
	$(CC) -o $@ $(CFLAGS) passwd.o $(EXTRALIBS)
	$(FASTRAMPRG) passwd

clean:
	$(RM) $(PROGRAMS) *.o

install: $(PROGRAMS)
	$(CP) passwd $(USRBINDIR)/passwd$(TTPEXT)
	$(CHMOD) 4755 $(USRBINDIR)/passwd$(TTPEXT)
