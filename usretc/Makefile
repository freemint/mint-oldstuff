SUBDIRS=ac syslog halt

all: subdirs

subdirs:
	for file in $(SUBDIRS) ; do cd $$file ; echo $$file ; $(MAKE); cd .. ; done

install:
	for file in $(SUBDIRS) ; do cd $$file ; echo $$file ; $(MAKE) install ; cd .. ; done

clean:
	for file in $(SUBDIRS) ; do cd $$file ; echo $$file ; $(MAKE) clean ; cd .. ; done
