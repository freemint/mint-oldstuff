# Makefile for init

include ../../.builddir
include ../../MakeVars

CFLAGS= $(ADDITIONALCFLAGS)
CPPFLAGS = -I$(BUILDDIR)/include -DMINT -DSYNC_FILESYS

EXTRALIBS = $(BUILDDIR)/lib/libusers.a

init: init.o $(EXTRALIBS)
	$(CC) -o $@ $(CFLAGS) init.o $(EXTRALIBS)
	$(FASTRAMPRG) init

init.o: version.h init.c

version.h: $(BUILDDIR)/etc/motd
	@$(ECHO) \#define MINTOSVERS \"`cat $(BUILDDIR)/etc/motd`\" > version.h

clean:
	$(RM) init *.o

install: init
	-$(MV) $(USRSBINDIR)/init$(PRGEXT) $(USRSBINDIR)/init.old
	$(CP) init $(USRSBINDIR)/init$(PRGEXT)
	$(CHMOD) 0755 $(USRSBINDIR)/init$(PRGEXT)
