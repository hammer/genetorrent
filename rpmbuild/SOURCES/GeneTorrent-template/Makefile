BINDIR=/usr/bin
MANDIR=/usr/share/man/man1
DATADIR=/usr/share/GeneTorrent
INITDIR=/etc/rc.d/init.d
CONFDIR=/etc/gnos.d
PYTHONSITEDIR24=/usr/lib/python2.4/site-packages
PYTHONSITEDIR26=/usr/lib/python2.6/site-packages

ifdef RPM_DOC_DIR
installDir=$(RPM_BUILD_ROOT)$(BINDIR)
manDir=$(RPM_BUILD_ROOT)$(MANDIR)
dataDir=$(RPM_BUILD_ROOT)$(DATADIR)
initDir=$(RPM_BUILD_ROOT)$(INITDIR)
confDir=$(RPM_BUILD_ROOT)$(CONFDIR)
pythonSiteDir24=$(RPM_BUILD_ROOT)$(PYTHONSITEDIR24)
pythonSiteDir26=$(RPM_BUILD_ROOT)$(PYTHONSITEDIR26)
else
installDir=$(BINDIR)
manDir=$(MANDIR)
dataDir=$(DATADIR)
initDir=$(INITDIR)
confDir=$(CONFDIR)
pythonSiteDir24=$(PYTHONSITEDIR24)
pythonSiteDir26=$(PYTHONSITEDIR26)
endif

all:

clean:

install:
	mkdir -p $(dataDir)
	mkdir -p $(initDir)
	mkdir -p $(pythonSiteDir)
	mkdir -p $(confDir)
        
	mkdir -p $(pythonSiteDir24)
	mkdir -p $(pythonSiteDir26)
	cp $(BINDIR)/GeneTorrent $(installDir)
	cp $(BINDIR)/gtoinfo $(installDir)
	cp $(BINDIR)/gtocheck $(installDir)
	cp $(BINDIR)/GTLoadBalancer $(installDir)
	cp /usr/share/GeneTorrent/dhparam.pem $(dataDir)
	cp /usr/share/man/man1/GeneTorrent.1 $(manDir)
	cp $(INITDIR)/GeneTorrent $(initDir)
	cp $(INITDIR)/GTLoadBalancer $(initDir)
	cp $(CONFDIR)/GeneTorrent.conf $(confDir)
	cp $(CONFDIR)/GTLoadBalancer.conf $(confDir)
	(test -e $(PYTHONSITEDIR24)/gtoinfo.py && cp $(PYTHONSITEDIR24)/gtoinfo.py $(pythonSiteDir24)) || true
	(test -e $(PYTHONSITEDIR26)/gtoinfo.py && cp $(PYTHONSITEDIR26)/gtoinfo.py $(pythonSiteDir26)) || true

