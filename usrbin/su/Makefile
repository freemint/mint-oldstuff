include ../../.builddir
include ../../MakeVars

CPPFLAGS = -I$(BUILDDIR)/include -DMINT
CFLAGS = $(ADDITIONALCFLAGS)
EXTRALIBS = $(BUILDDIR)/lib/libusers.a $(BUILDDIR)/usretc/syslog/libsyslog.a

TARGET = su
OBJS = su.o $(EXTRALIBS)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)
	$(FASTRAMPRG) $(TARGET)

su.o: su.c

clean:
	$(RM) $(TARGET) *.o

install: $(TARGET)
	$(CP) $(TARGET) $(USRBINDIR)/$(TARGET)$(TTPEXT)
	$(CHMOD) 4755 $(USRBINDIR)/$(TARGET)$(TTPEXT)
