include MakeVars

SUBDIRS=usretc usrsbin usrbin usrucb

all: .builddir announce lib subdirs finish

.builddir:
	@bin/echo BUILDDIR = $(PWD) > .builddir

announce:
	@$(SHELL) makever.sh
	@$(ECHO)
	@$(ECHO) "Starting build of MiNTOS Release "`cat .relnum`" ("$(CONFIGNAME)") #"`cat .version`": "`date`
	@$(ECHO)
	@$(ECHO) "MiNTOS Release "`cat .relnum`" ("$(CONFIGNAME)") #"`cat .version`": "`date` > etc/motd
	@$(ECHO) >> etc/motd
#
	@$(ECHO) "This is MiNTOS Release "`cat .relnum` > Version
	@$(ECHO) "config name: "$(CONFIGNAME) >> Version
	@$(ECHO) "build number: "`cat .version` >> Version
	@$(ECHO) "Dated: "`date` >> Version

finish:
	@$(ECHO)
	@$(ECHO) "Finished build of MiNTOS Release "`cat .relnum`" ("$(CONFIGNAME)") #"`cat .version`": "`date`
	@$(ECHO)
	@$(ECHO) 'Become root (superuser), copy the files from the "etc" directory'
	@$(ECHO) 'into /etc and then type "make install" to complete the installation.'
	@$(ECHO)

lib: lib/libusers.a

lib/libusers.a: lib/cfsetspeed.c lib/chroot.c lib/daemon.c lib/fcrypt.c lib/getloadavg.c lib/getpass.c lib/getttyent.c lib/getusershell.c lib/initgrp.c lib/itimer.c lib/login.c lib/login_tty.c lib/logout.c lib/logwtmp.c lib/mint.c lib/pty.c lib/pwcache.c lib/readv.c lib/setenv.c lib/sigvec.c lib/strcasecmp.c lib/strncasecmp.c lib/strsep.c lib/syslog.c lib/utmp.c lib/writev.c lib/wtmp.c
	@cd lib ; $(MAKE)

subdirs:
	@for file in $(SUBDIRS) ; do cd $$file ; $(MAKE); cd .. ; done

install: .builddir
	@for file in $(SUBDIRS) ; do cd $$file ; $(MAKE) install; cd .. ; done

man:
	$(CP) man/ac.8 /usr/man/man8
	$(CP) man/getty.8 /usr/man/man8
	$(CP) man/gettytab.5 /usr/man/man5
	$(CP) man/login.1 /usr/man/man1
	$(CP) man/su.1 /usr/man/man1
	$(CP) man/ttys.5 /usr/man/man5
	$(CP) man/wlogin.1 /usr/man/man1

clean:	.builddir
	@for file in $(SUBDIRS) ; do cd $$file ; $(MAKE) clean; cd .. ; done
	-@$(RM) .version .builddir
