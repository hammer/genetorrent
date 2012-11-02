#  Copyright (c) 2011, Annai Systems, Inc.
#  All rights reserved.
#  
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
# 
#  2. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#  
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE
# 
#  Created under contract by Cardinal Peak, LLC.   www.cardinalpeak.com

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = boost openssl libtorrent src scripts init.d tests dpkg

EXTRA_DIST =                     \
   LICENSE                       \
   libtorrent/configure.gnu      \
   GeneTorrent.spec              \
   autogen.sh                    \
   GeneTorrent.nsi

dist_doc_DATA =                  \
   LICENSE                       \
   README

# These can be overridden on the make command line.
DISABLE_RPM_CHECK = no
BUILD_VERSION =
WITH_BOOST =
RPMBUILDDIR = $(abs_top_builddir)/rpmbuild

# Some systems don't handle -D for _topdir (AmazonAMI???)
RPMBUILD_DEFINES = \
	--define '_topdir $(RPMBUILDDIR)' \
	--define '_disable_check $(DISABLE_RPM_CHECK)'

rpm: dist
	$(MAKE) rpmbuild-internal

rpmcheck: distcheck
	$(MAKE) rpmbuild-internal

rpmbuild-internal:
	mkdir -p $(RPMBUILDDIR)/SRPMS
	mkdir -p $(RPMBUILDDIR)/RPMS
	mkdir -p $(RPMBUILDDIR)/SPECS
	mkdir -p $(RPMBUILDDIR)/BUILD
	mkdir -p $(RPMBUILDDIR)/SOURCES
	IFS=$$'\n'; \
		if [ -n "$(BUILD_VERSION)" ]; then \
			DEF_BLD_VER=--define='build_version $(BUILD_VERSION)'; fi; \
		if [ -n "$(WITH_BOOST)" ]; then \
			DEF_WITH_BOOST=--define='_with_boost $(WITH_BOOST)'; fi; \
		set -x; \
		rpmbuild $(RPMBUILD_DEFINES) $${DEF_BLD_VER} $${DEF_WITH_BOOST} \
			--showrc > $(RPMBUILDDIR)/gt-rpmrc.out; \
		rpmbuild $(RPMBUILD_DEFINES) $${DEF_BLD_VER} $${DEF_WITH_BOOST} \
			-ta $(distdir).tar.gz

windows-installer: all
	/cygdrive/c/Program\ Files/NSIS/makensis.exe GeneTorrent.nsi || \
			/cygdrive/c/Program\ Files\ \(x86\)/NSIS/makensis.exe GeneTorrent.nsi

DEB_CODENAME := $(shell lsb_release -cs)

.PHONY: deb deb-builder deb-precise deb-squeeze deb-oneiric
deb-precise:
	$(MAKE) DEB_DISTRO_TAG=0ubuntu1 deb-builder

deb-squeeze:
	$(MAKE) DEB_DISTRO_TAG=1 deb-builder

deb-oneiric:
	$(MAKE) DEB_DISTRO_TAG=0ubuntu1 deb-builder

deb: deb-$(DEB_CODENAME)

REPLACE_VARS = \
		-e 's|[@]DEB_DISTRO_TAG@|$(DEB_DISTRO_TAG)|g' \
		-e 's|[@]DEB_CODENAME@|$(DEB_CODENAME)|g' \
		-e 's|[@]VERSION@|$(VERSION)|g'

deb-builder:
	$(MAKE) dist
	tar xzf $(PACKAGE)-$(VERSION).tar.gz
	mv $(PACKAGE)-$(VERSION) $(PACKAGE)-$(VERSION).orig
	tar xzf $(PACKAGE)-$(VERSION).tar.gz
	mv $(PACKAGE)-$(VERSION).tar.gz genetorrent_$(VERSION).orig.tar.gz
	mkdir $(PACKAGE)-$(VERSION)/debian
	cp -a $(top_srcdir)/dpkg/common/* $(PACKAGE)-$(VERSION)/debian
	cp -a $(top_srcdir)/dpkg/$(DEB_CODENAME)/* $(PACKAGE)-$(VERSION)/debian
	$(SED) -i $(REPLACE_VARS) $(PACKAGE)-$(VERSION)/debian/control
	$(SED) -i $(REPLACE_VARS) $(PACKAGE)-$(VERSION)/debian/changelog
	cd $(PACKAGE)-$(VERSION) && debuild --prepend-path=/usr/lib/ccache -us -uc

