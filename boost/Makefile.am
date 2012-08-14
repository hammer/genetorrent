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

BOOSTVER = 1.48.0
LEXT = .so.$(BOOSTVER)

all:
	mkdir -p lib include

install:
	@echo "Installing boost libraries into $(DESTDIR)$(pkglibdir)"
	test ! -f $(srcdir)/lib/libboost_filesystem$(LEXT) || \
		$(INSTALL_SCRIPT) -D -m 644 $(srcdir)/lib/libboost_filesystem$(LEXT) \
		$(DESTDIR)$(pkglibdir)/libboost_filesystem$(LEXT)
	test ! -f $(srcdir)/lib/libboost_filesystem$(LEXT) || \
		(cd $(DESTDIR)$(pkglibdir) && rm -f libboost_filesystem.so && \
			$(LN_S) libboost_filesystem$(LEXT) libboost_filesystem.so)
	test ! -f $(srcdir)/lib/libboost_program_options$(LEXT) || \
		$(INSTALL_SCRIPT) -D -m 644 $(srcdir)/lib/libboost_program_options$(LEXT) \
		$(DESTDIR)$(pkglibdir)/libboost_program_options$(LEXT)
	test ! -f $(srcdir)/lib/libboost_program_options$(LEXT) || \
		(cd $(DESTDIR)$(pkglibdir) && rm -f libboost_program_options.so && \
			$(LN_S) libboost_program_options$(LEXT) libboost_program_options.so)
	test ! -f $(srcdir)/lib/libboost_regex$(LEXT) || \
		$(INSTALL_SCRIPT) -D -m 644 $(srcdir)/lib/libboost_regex$(LEXT) \
		$(DESTDIR)$(pkglibdir)/libboost_regex$(LEXT)
	test ! -f $(srcdir)/lib/libboost_regex$(LEXT) || \
		(cd $(DESTDIR)$(pkglibdir) && rm -f libboost_regex.so && \
			$(LN_S) libboost_regex$(LEXT) libboost_regex.so)
	test ! -f $(srcdir)/lib/libboost_system$(LEXT) || \
		$(INSTALL_SCRIPT) -D -m 644 $(srcdir)/lib/libboost_system$(LEXT) \
		$(DESTDIR)$(pkglibdir)/libboost_system$(LEXT)
	test ! -f $(srcdir)/lib/libboost_system$(LEXT) || \
		(cd $(DESTDIR)$(pkglibdir) && rm -f libboost_system.so && \
			$(LN_S) libboost_system$(LEXT) libboost_system.so)

clean:
	rm -rf boost_1_48_0 include lib *.tar.gz

.PHONY: install clean

