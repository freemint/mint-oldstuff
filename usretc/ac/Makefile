CFLAGS = $(ADDITIONALCFLAGS)

ac: ac.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o ac ac.c
	../../bin/ttramify ac

clean:
	/bin/rm ac

install: ac
	cp ac $(USRETCDIR)/ac$(TTPEXT)
	chmod 0755 $(USRETCDIR)/ac$(TTPEXT)
