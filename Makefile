# This is where mint.prg sits, init.prg should be in the same place unless
#   you have directed mint to look elsewhere using a mint.cnf file.

MINTDIR=/c/mint

# This is where all your executables are held.

USRSBINDIR=/usr/sbin
USRBINDIR=/usr/bin
USRUCBDIR=/usr/ucb
USRETCDIR=/usr/etc

# This is the directory which will be come /etc using a symbolic link set up
# in your mint.cnf. For OS's other than MiNT leave it as /etc.

ETCDIR=/etc

# This is the extention given to executables which will be called from the
# command line.

TTPEXT=

# This is the extention given to init, MiNT expects it to be .prg.

PRGEXT=

SHELL=/bin/sh
ADDITIONALCFLAGS=-O2
#CC=cgcc
#AR=car
CC=cc
AR=ar
MAKE=make CC=$(CC) AR=$(AR) ADDITIONALCFLAGS="$(ADDITIONALCFLAGS)" \
	MINTDIR=$(MINTDIR) USRSBINDIR=$(USRSBINDIR) \
	USRBINDIR=$(USRBINDIR) USRUCBDIR=$(USRUCBDIR) \
	USRETCDIR=$(USRETCDIR) ETCDIR=$(ETCDIR)

SUBDIRS=usretc usrsbin usrbin usrucb

all: lib subdirs

lib: lib/libusers.a

lib/libusers.a: lib/utmp.c lib/wtmp.c
	cd lib ; $(MAKE)

subdirs:
	for file in $(SUBDIRS) ; do cd $$file ; echo $$file ; $(MAKE); cd .. ; done

install:
	for file in $(SUBDIRS) ; do cd $$file ; echo $$file ; $(MAKE) install; cd .. ; done

man:
	cp man/ac.8 /usr/man/man8
	cp man/getty.8 /usr/man/man8
	cp man/gettytab.5 /usr/man/man5
	cp man/login.1 /usr/man/man1
	cp man/su.1 /usr/man/man1
	cp man/ttys.5 /usr/man/man5
	cp man/wlogin.1 /usr/man/man1

clean:
	for file in $(SUBDIRS) ; do cd $$file ; echo $$file ; $(MAKE) clean; cd .. ; done
