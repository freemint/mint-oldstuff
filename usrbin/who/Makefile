include ../../.builddir
include ../../MakeVars

CFLAGS = $(ADDITIONALCFLAGS)

who: who.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o who who.c
	$(FASTRAMPRG) who

clean:
	$(RM) who

install: who
	$(CP) who $(USRBINDIR)/who$(TTPEXT)
	$(CHMOD) 0755 $(USRBINDIR)/who$(TTPEXT)
