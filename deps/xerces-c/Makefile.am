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

EXTRA_DIST = buildGeneTorrentXerces-C.sh

LIBVER = 3.1

if DARWIN
LEXT = -$(LIBVER).dylib
LN_LEXT = .dylib
else
LEXT = -$(LIBVER).so
LN_LEXT = .so
endif

all:
	mkdir -p lib include bin

install:
	@echo "Installing xerces-c libraries into $(DESTDIR)$(pkglibdir)"
	@set -e; echo "  INSTALL -> xerces-c"; \
	test ! -f $(srcdir)/lib/libxerces-c$(LEXT) || \
		$(INSTALL_SCRIPT) -D -m 644 $(srcdir)/lib/libxerces-c$(LEXT) \
		$(DESTDIR)$(pkglibdir)/libxerces-c$(LEXT); \
	test ! -f $(srcdir)/lib/libxerces-c$(LEXT) || \
		(cd $(DESTDIR)$(pkglibdir) && rm -f libxerces-c$(LN_LEXT) && \
			$(LN_S) libxerces-c$(LEXT) libxerces-c$(LN_LEXT));

.PHONY: install

