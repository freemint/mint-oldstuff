SUBDIRS=finger last talk wlogin

all: subdirs

subdirs:
	@for file in $(SUBDIRS) ; do cd $$file ; $(MAKE); cd .. ; done

install:
	@for file in $(SUBDIRS) ; do cd $$file ; $(MAKE) install ; cd .. ; done

clean:
	@for file in $(SUBDIRS) ; do cd $$file ; $(MAKE) clean ; cd .. ; done
