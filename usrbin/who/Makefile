CFLAGS = $(ADDITIONALCFLAGS)

who: who.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o who who.c
	../../bin/ttramify who

clean:
	/bin/rm who

install: who
	cp who $(USRBINDIR)/who$(TTPEXT)
	chmod 0755 $(USRBINDIR)/who$(TTPEXT)
