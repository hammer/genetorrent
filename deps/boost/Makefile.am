#  Copyright (c) 2012, Annai Systems, Inc.
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

EXTRA_DIST = buildGeneTorrentBoost.sh

if DARWIN
BOOSTVER = 1.48.0
LEXT = .dylib
else
BOOSTVER = 1.48.0
LEXT = .so.$(BOOSTVER)
endif

BOOST_INST_LIBS = filesystem program_options regex system thread

all:
	mkdir -p lib include

install:
	@echo "Installing boost libraries into $(DESTDIR)$(pkglibdir)"
if ! DARWIN
	@set -e; for lib in $(BOOST_INST_LIBS); do \
		echo "  INSTALL -> $${lib}"; \
		test ! -f $(srcdir)/lib/libboost_$${lib}$(LEXT) || \
			$(INSTALL_SCRIPT) -D -m 644 $(srcdir)/lib/libboost_$${lib}$(LEXT) \
			$(DESTDIR)$(pkglibdir)/libboost_$${lib}$(LEXT); \
		test ! -f $(srcdir)/lib/libboost_$${lib}$(LEXT) || \
			(cd $(DESTDIR)$(pkglibdir) && rm -f libboost_$${lib}.so && \
				$(LN_S) libboost_filesystem$(LEXT) libboost_$${lib}.so); \
	done
else
	@set -e; for lib in $(BOOST_INST_LIBS); do \
		echo "  INSTALL -> $${lib}"; \
		test ! -f $(srcdir)/lib/libboost_$${lib}$(LEXT) || \
			$(INSTALL_SCRIPT) -D -m 644 $(srcdir)/lib/libboost_$${lib}$(LEXT) \
			$(DESTDIR)$(pkglibdir)/libboost_$${lib}$(LEXT); \
	done
endif

.PHONY: install

