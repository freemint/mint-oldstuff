CFLAGS = $(ADDITIONALCFLAGS)
CPPFLAGS = -I../../include

halt: halt.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o halt halt.c ../syslog/libsyslog.a
	../../bin/ttramify halt

clean:
	/bin/rm halt

install: halt
	cp halt $(USRETCDIR)/halt$(TTPEXT)
	chmod 0755 $(USRETCDIR)/halt$(TTPEXT)
