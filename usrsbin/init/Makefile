# Makefile for init
CFLAGS= $(ADDITIONALCFLAGS)
CPPFLAGS = -I../../include -DMINT -DSYNC_FILESYS

EXTRALIBS = ../../lib/libusers.a

init: init.o $(EXTRALIBS)
	$(CC) -o $@ $(CFLAGS) init.o $(EXTRALIBS)
	../../bin/ttramify init

clean:
	/bin/rm init *.o

install: init
	-mv $(USRSBINDIR)/init$(PRGEXT) $(USRSBINDIR)/init.old
	cp init $(USRSBINDIR)/init$(PRGEXT)
	chmod 0755 $(USRSBINDIR)/init$(PRGEXT)
