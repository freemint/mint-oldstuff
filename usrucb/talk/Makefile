include ../../.builddir
include ../../MakeVars

CFLAGS = $(ADDITIONALCFLAGS)

talk: talk.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o talk talk.c -lcurses -ltermcap
	$(FASTRAMPRG) talk

clean:
	$(RM) talk

install: talk
	$(CP) talk $(USRUCBDIR)/talk$(TTPEXT)
	$(CHMOD) 0755 $(USRUCBDIR)/talk$(TTPEXT)
