include ../../.builddir
include ../../MakeVars

CFLAGS = $(ADDITIONALCFLAGS)

ac: ac.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o ac ac.c
	$(FASTRAMPRG) ac

clean:
	$(RM) ac

install: ac
	$(CP) ac $(USRETCDIR)/ac$(TTPEXT)
	$(CHMOD) 0755 $(USRETCDIR)/ac$(TTPEXT)
