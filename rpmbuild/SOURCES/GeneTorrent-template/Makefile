BINDIR=/usr/bin
MANDIR=/usr/share/man/man1
DATADIR=/usr/share/GeneTorrent
INITDIR=/etc/rc.d/init.d
PYTHONSITEDIR=/usr/lib/python2.6/site-packages
CONFDIR=/etc/gnos.d

ifdef RPM_DOC_DIR
installDir=$(RPM_BUILD_ROOT)$(BINDIR)
manDir=$(RPM_BUILD_ROOT)$(MANDIR)
dataDir=$(RPM_BUILD_ROOT)$(DATADIR)
initDir=$(RPM_BUILD_ROOT)$(INITDIR)
pythonSiteDir=$(RPM_BUILD_ROOT)$(PYTHONSITEDIR)
confDir=$(RPM_BUILD_ROOT)$(CONFDIR)
else
installDir=$(BINDIR)
manDir=$(MANDIR)
dataDir=$(DATADIR)
initDir=$(INITDIR)
pythonSiteDir=$(PYTHONSITEDIR)
confDir=$(CONFDIR)
endif

all:

clean:

install:
	mkdir -p $(dataDir)
	mkdir -p $(initDir)
	mkdir -p $(pythonSiteDir)
	mkdir -p $(confDir)
        
	cp $(BINDIR)/GeneTorrent $(installDir)
	cp $(BINDIR)/gtoinfo $(installDir)
	cp $(BINDIR)/gtocheck $(installDir)
	cp $(BINDIR)/GTLoadBalancer $(installDir)
	cp $(PYTHONSITEDIR)/gtoinfo.py $(pythonSiteDir)
	cp $(PYTHONSITEDIR)/gtoinfo.pyc $(pythonSiteDir)
	cp $(PYTHONSITEDIR)/gtoinfo.pyo $(pythonSiteDir)
	cp /usr/share/GeneTorrent/dhparam.pem $(dataDir)
	cp /usr/share/man/man1/GeneTorrent.1 $(manDir)
	cp $(INITDIR)/GeneTorrent $(initDir)
	cp $(INITDIR)/GTLoadBalancer $(initDir)
	cp $(CONFDIR)/GeneTorrent.conf $(confDir)
	cp $(CONFDIR)/GTLoadBalancer.conf $(confDir)
