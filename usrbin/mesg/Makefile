CFLAGS = $(ADDITIONALCFLAGS)

mesg: mesg.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o mesg mesg.c
	../../bin/ttramify mesg

clean:
	/bin/rm mesg

install: mesg
	cp mesg $(USRBINDIR)/mesg$(TTPEXT)
	chmod 0755 $(USRBINDIR)/mesg$(TTPEXT)
