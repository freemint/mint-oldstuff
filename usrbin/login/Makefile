include ../../.builddir
include ../../MakeVars

CPPFLAGS = -I$(BUILDDIR)/include -DMINT
CFLAGS = $(ADDITIONALCFLAGS)
EXTRALIBS = $(BUILDDIR)/lib/libusers.a $(BUILDDIR)/usretc/syslog/libsyslog.a

TARGET = login
OBJS = login.o klogin.o $(EXTRALIBS)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)
	$(FASTRAMPRG) $(TARGET)

login.o: login.c pathnames.h
klogin.o: klogin.c pathnames.h

clean:
	$(RM) $(TARGET) *.o

install: $(TARGET)
	$(CP) $(TARGET) $(USRBINDIR)/$(TARGET)$(TTPEXT)
	$(CHMOD) 4755 $(USRBINDIR)/$(TARGET)$(TTPEXT)
