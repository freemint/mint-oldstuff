CFLAGS = $(ADDITIONALCFLAGS)

finger: finger.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o finger finger.c
	../../bin/ttramify finger

clean:
	/bin/rm finger

install: finger
	cp finger $(USRUCBDIR)/finger$(TTPEXT)
	chmod 0755 $(USRUCBDIR)/finger$(TTPEXT)
