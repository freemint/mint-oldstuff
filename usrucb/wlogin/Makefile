CFLAGS = $(ADDITIONALCFLAGS)
EXTRALIBS = ../../lib/libusers.a

wlogin: wlogin.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o wlogin wlogin.c $(EXTRALIBS)
	../../bin/ttramify wlogin

clean:
	/bin/rm wlogin

install: wlogin
	cp wlogin $(USRUCBDIR)/wlogin$(TTPEXT)
	chmod 0755 $(USRUCBDIR)/wlogin$(TTPEXT)
