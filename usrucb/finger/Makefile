include ../../.builddir
include ../../MakeVars

CFLAGS = $(ADDITIONALCFLAGS) -DREMOTE

finger: finger.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o finger finger.c
	$(FASTRAMPRG) finger

clean:
	$(RM) finger

install: finger
	$(CP) finger $(USRUCBDIR)/finger$(TTPEXT)
	$(CHMOD) 0755 $(USRUCBDIR)/finger$(TTPEXT)
