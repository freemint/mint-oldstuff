#
#	Makefile for a small porting library, (w) 1994, Kay Roemer.
#
#	Modified for MiNTOS by S.R.Usher
#

include ../.builddir
include ../MakeVars

CPPFLAGS = -I$(BUILDDIR)/include -DMINT -DLOGIN
#CFLAGS = -O2 -fomit-frame-pointer
CFLAGS = $(ADDITIONALCFLAGS)

#ifdef BASEREL
#all: bportlib.olb
#CFLAGS := $(CFLAGS) -mbaserel -mpcrel
#else
#all: portlib.olb
#endif

ifdef BASEREL
all: libusersb.a
CFLAGS := $(CFLAGS) -mbaserel -mpcrel
else
all: libusers.a
endif

#SRCS = mint.c sigvec.c daemon.c pty.c login_tty.c setenv.c syslog.c \
# initgrp.c chroot.c readv.c writev.c getusershell.c strncasecmp.c itimer.c \
# cfsetspeed.c logout.c logwtmp.c strsep.c strcasecmp.c fcrypt.c

SRCS =	cfsetspeed.c chroot.c daemon.c fcrypt.c getpass.c getttyent.c \
	getusershell.c initgrp.c itimer.c login_tty.c logout.c logwtmp.c \
	mint.c pty.c readv.c setenv.c sigvec.c strcasecmp.c strncasecmp.c \
	strsep.c syslog.c utmp.c writev.c wtmp.c getloadavg.c pwcache.c \
	login.c

OBJS = $(patsubst %.c, %.o, $(SRCS))

#portlib.olb: clean $(patsubst %.o, portlib.olb(%.o), $(OBJS))
#	$(AR) s $@

#bportlib.olb: clean $(patsubst %.o, bportlib.olb(%.o), $(OBJS))
#	$(AR) s $@

libusers.a: $(patsubst %.o, libusers.a(%.o), $(OBJS))
	$(AR) s $@

libusersb.a: $(patsubst %.o, libusersb.a(%.o), $(OBJS))
	$(AR) s $@

(%): %
	@$(AR) r $@ $%
	@$(RM) $%

clean:
	$(RM) -f $(OBJS)
