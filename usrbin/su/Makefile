CPPFLAGS = -I../../include -DMINT
CFLAGS = $(ADDITIONALCFLAGS)
EXTRALIBS = ../../lib/libusers.a ../../usretc/syslog/libsyslog.a

TARGET = su
OBJS = su.o

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(EXTRALIBS)
	../../bin/ttramify $(TARGET)

su.o: su.c

clean:
	/bin/rm $(TARGET) *.o

install: $(TARGET)
	cp $(TARGET) $(USRBINDIR)/$(TARGET)$(TTPEXT)
	chmod 4755 $(USRBINDIR)/$(TARGET)$(TTPEXT)
