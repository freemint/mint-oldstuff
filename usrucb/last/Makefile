CFLAGS = $(ADDITIONALCFLAGS)
CPPFLAGS = -DWTMP=\"/var/adm/wtmp\"

last: last.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o last last.c
	../../bin/ttramify last

clean:
	/bin/rm last

install: last
	cp last $(USRUCBDIR)/last$(TTPEXT)
	chmod 0755 $(USRUCBDIR)/last$(TTPEXT)
