CFLAGS = $(ADDITIONALCFLAGS)

talk: talk.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o talk talk.c -lcurses -ltermcap
	../../bin/ttramify talk

clean:
	/bin/rm talk

install: talk
	cp talk $(USRUCBDIR)/talk$(TTPEXT)
	chmod 0755 $(USRUCBDIR)/talk$(TTPEXT)
