include ../../.builddir
include ../../MakeVars

CFLAGS = $(ADDITIONALCFLAGS)

hostname: hostname.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o hostname hostname.c
	$(FASTRAMPRG) hostname

clean:
	$(RM) hostname

install: hostname
	$(CP) hostname $(USRBINDIR)/hostname$(TTPEXT)
	$(CHMOD) 0755 $(USRBINDIR)/hostname$(TTPEXT)
