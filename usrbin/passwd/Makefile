# Makefile for passwd

SHELL = /bin/sh

CPPFLAGS = -I../../include -DMINT -DLOGIN -DBZERO -DSHELL=\"$(SHELL)\" -DOWN_TIMEOUT
CFLAGS = -fwritable-strings -g $(ADDITIONALCFLAGS)

EXTRALIBS = ../../lib/libusers.a ../../usretc/syslog/libsyslog.a

#PROGRAMS=login passwd su
PROGRAMS=passwd

all: $(PROGRAMS)

login: login.o $(EXTRALIBS)
	$(CC) -o $@ $(CFLAGS) login.o $(EXTRALIBS)
	../../bin/ttramify login

su: su.o $(EXTRALIBS)
	$(CC) -o $@ $(CFLAGS) su.o $(EXTRALIBS)
	../../bin/ttramify su

passwd: passwd.o $(EXTRALIBS)
	$(CC) -o $@ $(CFLAGS) passwd.o $(EXTRALIBS)
	../../bin/ttramify passwd

clean:
	/bin/rm $(PROGRAMS) *.o

#install: $(PROGRAMS)
#	cp login $(USRBINDIR)/login$(TTPEXT)
#	chmod 4755 $(USRBINDIR)/login$(TTPEXT)
#	cp su $(USRBINDIR)/su$(TTPEXT)
#	chmod 4755 $(USRBINDIR)/su$(TTPEXT)
#	cp passwd $(USRBINDIR)/passwd$(TTPEXT)
#	chmod 4755 $(USRBINDIR)/passwd$(TTPEXT)

install: $(PROGRAMS)
	cp passwd $(USRBINDIR)/passwd$(TTPEXT)
	chmod 4755 $(USRBINDIR)/passwd$(TTPEXT)
