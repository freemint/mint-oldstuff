CFLAGS=$(ADDITIONALCFLAGS)
#CPPFLAGS = -DDEBUG
CPPFLAGS = -DMINT -DLOGIN

libusers.a: libusers.a(utmp.o) libusers.a(wtmp.o) libusers.a(writev.o) libusers.a(strcasecmp.o) libusers.a(fcrypt.o) libusers.a(getttyent.o) libusers.a(getusershell.o)
	ar s libusers.a
