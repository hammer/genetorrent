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

EXTRA_DIST = buildGeneTorrentOpenSSL.sh

OPENSSLVER = 1.0.0
LEXT = .so.$(OPENSSLVER)

all:
	mkdir -p lib include

install:
	@echo "Installing openssl libraries into $(DESTDIR)$(pkglibdir)"
	test ! -f $(srcdir)/lib/libcrypto$(LEXT) || \
		$(INSTALL_SCRIPT) -D -m 644 $(srcdir)/lib/libcrypto$(LEXT) \
		$(DESTDIR)$(pkglibdir)/libcrypto$(LEXT)
	test ! -f $(srcdir)/lib/libcrypto$(LEXT) || \
      (cd $(DESTDIR)$(pkglibdir) && rm -f libcrypto.so && \
         $(LN_S) libcrypto$(LEXT) libcrypto.so)
	test ! -f $(srcdir)/lib/libssl$(LEXT) || \
		$(INSTALL_SCRIPT) -D -m 644 $(srcdir)/lib/libssl$(LEXT) \
		$(DESTDIR)$(pkglibdir)/libssl$(LEXT)
	test ! -f $(srcdir)/lib/libssl$(LEXT) || \
      (cd $(DESTDIR)$(pkglibdir) && rm -f libssl.so && \
         $(LN_S) libssl$(LEXT) libssl.so)

clean:
	rm -rf bin certs lib man openssl-1.0.1c* openssl.cnf include misc private

.PHONY: install clean

