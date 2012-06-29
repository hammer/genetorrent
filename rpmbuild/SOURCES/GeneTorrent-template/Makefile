BINDIR=/usr/bin
MANDIR=/usr/share/man/man1
DATADIR=/usr/share/GeneTorrent
INITDIR=/etc/rc.d/init.d

ifdef RPM_DOC_DIR
installDir=$(RPM_BUILD_ROOT)$(BINDIR)
manDir=$(RPM_BUILD_ROOT)$(MANDIR)
dataDir=$(RPM_BUILD_ROOT)$(DATADIR)
initDir=$(RPM_BUILD_ROOT)$(INITDIR)
else
installDir=$(BINDIR)
manDir=$(MANDIR)
dataDir=$(DATADIR)
initDir=$(INITDIR)
endif

all:

clean:

install:
	mkdir -p $(dataDir)
	mkdir -p $(initDir)
	cp $(BINDIR)/GeneTorrent $(installDir)
	cp $(BINDIR)/gtoinfo $(installDir)
	cp $(BINDIR)/GTLoadBalancer $(installDir)
	cp /usr/share/GeneTorrent/dhparam.pem $(dataDir)
	cp /usr/share/man/man1/GeneTorrent.1 $(manDir)
	cp /etc/rc.d/init.d/GeneTorrent $(initDir)
	cp /etc/rc.d/init.d/GTLoadBalancer $(initDir)
