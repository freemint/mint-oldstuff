include ../../.builddir
include ../../MakeVars

CFLAGS = $(ADDITIONALCFLAGS)
CPPFLAGS = -DWTMP=\"/var/adm/wtmp\" -DRLOGIN

last: last.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o last last.c
	$(FASTRAMPRG) last

clean:
	$(RM) last

install: last
	$(CP) last $(USRUCBDIR)/last$(TTPEXT)
	$(CHMOD) 0755 $(USRUCBDIR)/last$(TTPEXT)
