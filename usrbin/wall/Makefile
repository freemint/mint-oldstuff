include ../../.builddir
include ../../MakeVars

CFLAGS=$(ADDITIONALCFLAGS)

wall: wall.c
	$(CC) $(CFLAGS) wall.c -o wall

install: wall
	$(CP) wall $(USRBINDIR)/wall$(TTPEXT)
	$(CHGRP) tty $(USRBINDIR)/wall$(TTPEXT)
	$(CHMOD) 2711 $(USRBINDIR)/wall$(TTPEXT)

clean:
	$(RM) wall
