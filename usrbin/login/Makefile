CPPFLAGS = -I../../include -DMINT
CFLAGS = $(ADDITIONALCFLAGS)
EXTRALIBS = ../../lib/libusers.a ../../usretc/syslog/libsyslog.a

TARGET = login
OBJS = login.o klogin.o

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(EXTRALIBS)
	../../bin/ttramify $(TARGET)

login.o: login.c pathnames.h
klogin.o: klogin.c pathnames.h

clean:
	/bin/rm $(TARGET) *.o

install: $(TARGET)
	cp $(TARGET) $(USRBINDIR)/$(TARGET)$(TTPEXT)
	chmod 4755 $(USRBINDIR)/$(TARGET)$(TTPEXT)
