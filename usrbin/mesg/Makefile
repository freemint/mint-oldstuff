include ../../.builddir
include ../../MakeVars

CFLAGS = $(ADDITIONALCFLAGS)

mesg: mesg.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o mesg mesg.c
	$(FASTRAMPRG) mesg

clean:
	$(RM) -f mesg

install: mesg
	$(CP) mesg $(USRBINDIR)/mesg$(TTPEXT)
	$(CHMOD) 0755 $(USRBINDIR)/mesg$(TTPEXT)
