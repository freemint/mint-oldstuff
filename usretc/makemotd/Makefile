include ../../.builddir
include ../../MakeVars

CFLAGS = $(ADDITIONALCFLAGS)

makemotd: makemotd.c version.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o makemotd makemotd.c
	$(FASTRAMPRG) makemotd

version.h: $(BUILDDIR)/etc/motd
	@$(ECHO) \#define MINTOSVERS \"`cat $(BUILDDIR)/etc/motd`\" > version.h

clean:
	$(RM) makemotd

install: makemotd
	$(CP) makemotd $(USRETCDIR)/makemotd$(TTPEXT)
	$(CHMOD) 0755 $(USRETCDIR)/makemotd$(TTPEXT)
