include ../../.builddir
include ../../MakeVars

CFLAGS = $(ADDITIONALCFLAGS)
CPPFLAGS = -I$(BUILDDIR)/include

halt: halt.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o halt halt.c ../syslog/libsyslog.a
	$(FASTRAMPRG) halt

clean:
	$(RM) halt

install: halt
	$(CP) halt $(USRETCDIR)/halt$(TTPEXT)
	$(CHMOD) 0755 $(USRETCDIR)/halt$(TTPEXT)
