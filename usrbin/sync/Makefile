include ../../.builddir
include ../../MakeVars

CFLAGS = $(ADDITIONALCFLAGS)

sync: sync.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o sync sync.c
	$(FASTRAMPRG) sync

clean:
	$(RM) sync

install: sync
	$(CP) sync $(USRBINDIR)/sync$(TTPEXT)
	$(CHMOD) 0755 $(USRBINDIR)/sync$(TTPEXT)
