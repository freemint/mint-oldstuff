include ../../.builddir
include ../../MakeVars

CFLAGS = $(ADDITIONALCFLAGS)
EXTRALIBS = $(BUILDDIR)/lib/libusers.a

wlogin: wlogin.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o wlogin wlogin.c $(EXTRALIBS)
	$(FASTRAMPRG) wlogin

clean:
	$(RM) wlogin

install: wlogin
	$(CP) wlogin $(USRUCBDIR)/wlogin$(TTPEXT)
	$(CHMOD) 0755 $(USRUCBDIR)/wlogin$(TTPEXT)
