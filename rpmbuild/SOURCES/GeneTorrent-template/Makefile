BINDIR=/usr/bin
MANDIR=/usr/share/man/man1
DATADIR=/usr/share/GeneTorrent
INITDIR=/etc/rc.d/init.d
PYTHONSITEDIR24=/usr/lib/python2.4/site-packages
PYTHONSITEDIR26=/usr/lib/python2.6/site-packages
CONFDIR=/etc/gnos.d

ifdef RPM_DOC_DIR
installDir=$(RPM_BUILD_ROOT)$(BINDIR)
manDir=$(RPM_BUILD_ROOT)$(MANDIR)
dataDir=$(RPM_BUILD_ROOT)$(DATADIR)
initDir=$(RPM_BUILD_ROOT)$(INITDIR)
pythonSiteDir24=$(RPM_BUILD_ROOT)$(PYTHONSITEDIR24)
pythonSiteDir26=$(RPM_BUILD_ROOT)$(PYTHONSITEDIR26)
confDir=$(RPM_BUILD_ROOT)$(CONFDIR)
else
installDir=$(BINDIR)
manDir=$(MANDIR)
dataDir=$(DATADIR)
initDir=$(INITDIR)
pythonSiteDir=$(PYTHONSITEDIR)
pythonSiteDir24=$(PYTHONSITEDIR24)
pythonSiteDir26=$(PYTHONSITEDIR26)
confDir=$(CONFDIR)
endif

centos5=$(shell lsb_release -r | grep -ic "CentOS release 5")

all:

clean:

install:
	mkdir -p $(dataDir)
	mkdir -p $(initDir)
	mkdir -p $(pythonSiteDir24)
	mkdir -p $(pythonSiteDir26)
	mkdir -p $(confDir)

	# Common files
	cp $(BINDIR)/GeneTorrent $(installDir)
	cp $(BINDIR)/gtoinfo $(installDir)
	cp $(PYTHONSITEDIR)/gtoinfo.py $(pythonSiteDir)
	cp $(PYTHONSITEDIR)/gtoinfo.pyc $(pythonSiteDir)
	cp $(PYTHONSITEDIR)/gtoinfo.pyo $(pythonSiteDir)
	cp /usr/share/GeneTorrent/dhparam.pem $(dataDir)
	cp /usr/share/man/man1/GeneTorrent.1 $(manDir)
	cp $(INITDIR)/GeneTorrent $(initDir)
	cp $(CONFDIR)/GeneTorrent.conf $(confDir)

	# Files only packaged for CentOS 6.x / RHEL 6.x
	@if [ $(centos5) -ne 1 ]; then (test -e $(INITDIR)/GTLoadBalancer && cp $(INITDIR)/GTLoadBalancer $(initDir)) || true; fi
	@if [ $(centos5) -ne 1 ]; then (test -e $(BINDIR)/gtocheck && cp $(BINDIR)/gtocheck $(installDir)) || true; fi
	@if [ $(centos5) -ne 1 ]; then (test -e $(BINDIR)/GTLoadBalancer && cp $(BINDIR)/GTLoadBalancer $(installDir)) || true; fi
	@if [ $(centos5) -ne 1 ]; then (test -e $(CONFDIR)/GTLoadBalancer.conf && cp $(CONFDIR)/GTLoadBalancer.conf $(confDir)) || true; fi

	# Files with different locations in CentOS 5 and CentOS 6
	(test -e $(PYTHONSITEDIR24)/gtoinfo.py && cp $(PYTHONSITEDIR24)/gtoinfo.py $(pythonSiteDir24)) || true
	(test -e $(PYTHONSITEDIR26)/gtoinfo.py && cp $(PYTHONSITEDIR26)/gtoinfo.py $(pythonSiteDir26)) || true

