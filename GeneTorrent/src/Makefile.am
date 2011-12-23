################################################################################
# 
# Copyright 2011 Annai Systems, Inc.   All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without modification, 
# are permitted provided that the following conditions are met:
# 
#    1. Redistributions of source code must retain the above copyright notice, 
#       this list of conditions and the following disclaimer.
# 
#    2. Redistributions in binary form must reproduce the above copyright notice, 
#       this list of conditions and the following disclaimer in the documentation 
#       and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY ANNAI SYSTEMS ''AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
# OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# The views and conclusions contained in the software and documentation are those 
# of the authors and should not be interpreted as representing official policies, 
# either expressed or implied, of Annai Systems, Inc.
#
# Created under contract by Cardinal Peak, LLC.   www.cardinalpeak.com
#
################################################################################

bin_PROGRAMS = GeneTorrent

GeneTorrent_SOURCES = \
	geneTorrent.cpp \
	gtMain.cpp \
	stringTokenizer.cpp \
	tclapOutput.cpp \
	geneTorrentUtils.cpp

GTdatadir = $(datadir)/GeneTorrent

GTdata_DATA = \
	dhparam.pem \
	GeneTorrent.openssl.conf

GeneTorrent_CPPFLAGS = $(torrentrasterbar_CFLAGS)

GeneTorrent_LDADD = /usr/lib64/libtorrent-rasterbar.a 

man_MANS = GeneTorrent.1
