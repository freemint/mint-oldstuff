CFLAGS = $(ADDITIONALCFLAGS)

sync: sync.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o sync sync.c
	../../bin/ttramify sync

clean:
	/bin/rm sync

install: sync
	cp sync $(USRBINDIR)/sync$(TTPEXT)
	chmod 0755 $(USRBINDIR)/sync$(TTPEXT)
