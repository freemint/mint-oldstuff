include ../../.builddir
include ../../MakeVars

CFLAGS = $(ADDITIONALCFLAGS)

write: write.o myaccess.o
	$(CC) $(CFLAGS) write.o myaccess.o -o write
	$(FASTRAMPRG) write

clean:
	$(RM) -f write

install: write
	$(CP) write $(USRBINDIR)/write$(TTPEXT)
	$(CHGRP) tty $(USRBINDIR)/write$(TTPEXT)
	$(CHMOD) 2711 $(USRBINDIR)/write$(TTPEXT)
