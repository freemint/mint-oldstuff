CFLAGS = $(ADDITIONALCFLAGS)

write: write.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o write write.c
	../../bin/ttramify write

clean:
	/bin/rm write

install: write
	cp write $(USRBINDIR)/write$(TTPEXT)
	chmod 0755 $(USRBINDIR)/write$(TTPEXT)
