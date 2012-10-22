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

noinst_HEADERS =                 \
   accumulator.hpp               \
   geneTorrentUtils.h            \
   gtBase.h                      \
   gtDefs.h                      \
   gtDownload.h                  \
   gtLog.h                       \
   gtServer.h                    \
   gtUpload.h                    \
   gtUtils.h                     \
   gtNullStorage.h               \
   gtZeroStorage.h               \
   loggingmask.h                 \
   stringTokenizer.h

boost_LIBS = $(BOOST_SYSTEM_LIB)          \
             $(BOOST_FILESYSTEM_LIB)      \
             $(BOOST_REGEX_LIB)           \
             $(BOOST_PROGRAM_OPTIONS_LIB)

torrentrasterbar_CXXFLAGS =  $(shell $(PKG_CONFIG) --cflags ../libtorrent/libtorrent-rasterbar.pc)
torrentrasterbar_LIBS = -L../libtorrent/src/.libs $(shell $(PKG_CONFIG) --libs-only-l ../libtorrent/libtorrent-rasterbar.pc)

lib_LTLIBRARIES = libgenetorrent.la

libgenetorrent_la_SOURCES = gtBase.cpp \
                            gtDownload.cpp \
                            gtUtils.cpp \
                            gtLog.cpp \
                            gtAlerts.cpp \
                            geneTorrentUtils.cpp \
                            stringTokenizer.cpp \
                            gtNullStorage.cpp \
                            gtZeroStorage.cpp

libgenetorrent_la_CPPFLAGS = $(BOOST_CPPFLAGS) \
                             $(OPENSSL_INCLUDES)

libgenetorrent_la_CXXFLAGS = $(torrentrasterbar_CXXFLAGS) \
                             -I$(top_srcdir)/libtorrent/include

libgenetorrent_la_LDFLAGS = -version-info 0:0:0 \
                            -shared \
                            -no-undefined \
                            $(torrentrasterbar_LIBS) \
                            -R $(libdir)/GeneTorrent \
                            -L../libtorrent/src/.libs \
                            $(BOOST_LDFLAGS) \
                            $(OPEENSSL_LDFLAGS)

libgenetorrent_la_LIBADD = $(boost_LIBS)

bin_SCRIPTS = GeneTorrent

bin_PROGRAMS = gtupload    \
               gtdownload  \
               gtserver

gtupload_SOURCES = gtMain.cpp   \
                   gtUpload.cpp

gtdownload_SOURCES = gtMain.cpp

gtserver_SOURCES = gtMain.cpp   \
                   gtServer.cpp

GTdatadir = $(datadir)/GeneTorrent

dist_GTdata_DATA = dhparam.pem

common_ldflags     = $(torrentrasterbar_LIBS) \
                      -Wl,-rpath=$(libdir)/GeneTorrent \
                      $(BOOST_LDFLAGS) \
                      $(OPEENSSL_LDFLAGS)

common_ldadd      =  $(boost_LIBS)     \
                     -lssl             \
                     -lcrypto          \
                     libgenetorrent.la

gtupload_CPPFLAGS = $(BOOST_CPPFLAGS) \
                    $(OPENSSL_INCLUDES)

gtupload_CXXFLAGS = $(torrentrasterbar_CXXFLAGS) \
                    -I$(top_srcdir)/libtorrent/include \
                    -DGENETORRENT_UPLOAD

gtupload_LDFLAGS = $(common_ldflags)

gtupload_LDADD = $(common_ldadd)

gtdownload_CPPFLAGS = $(BOOST_CPPFLAGS) \
                      $(OPENSSL_INCLUDES)

gtdownload_CXXFLAGS = $(torrentrasterbar_CXXFLAGS) \
                      -I$(top_srcdir)/libtorrent/include \
                      -DGENETORRENT_DOWNLOAD

gtdownload_LDFLAGS = $(common_ldflags)

gtdownload_LDADD = $(common_ldadd)

gtserver_CPPFLAGS = $(BOOST_CPPFLAGS) \
                    $(OPENSSL_INCLUDES)

gtserver_CXXFLAGS = $(torrentrasterbar_CXXFLAGS) \
                   -I$(top_srcdir)/libtorrent/include \
                   -DGENETORRENT_SERVER

gtserver_LDFLAGS = $(common_ldflags)

gtserver_LDADD = $(common_ldadd)

dist_man_MANS = GeneTorrent.1 \
                gtdownload.1  \
                gtserver.1    \
                gtupload.1

EXTRA_DIST = man_gtcommon.1         \
             man_gtdownload.1       \
             man_gtserver.1         \
             man_gtupload.1         \
             man_GeneTorrent.1      \
             GeneTorrent_manual.txt \
             gtdownload_manual.txt  \
             gtserver_manual.txt    \
             gtupload_manual.txt    \
             GeneTorrent

CLEANFILES = gtdownload.1           \
             gtserver.1             \
             gtupload.1             \
             GeneTorrent.1          \
             GeneTorrent_manual.txt \
             gtdownload_manual.txt  \
             gtserver_manual.txt    \
             gtupload_manual.txt

gtdownload.1: $(srcdir)/man_gtdownload.1 $(srcdir)/man_gtcommon.1
	cat $^ > $@
if CYGWIN
	MANWIDTH=80 man -P cat ./$@ | col -b | unix2dos > gtdownload_manual.txt
endif

gtserver.1: $(srcdir)/man_gtserver.1 $(srcdir)/man_gtcommon.1
	cat $^ > $@
if CYGWIN
	MANWIDTH=80 man -P cat ./$@ | col -b | unix2dos > gtserver_manual.txt
endif

gtupload.1: $(srcdir)/man_gtupload.1 $(srcdir)/man_gtcommon.1
	cat $^ > $@
if CYGWIN
	MANWIDTH=80 man -P cat ./$@ | col -b | unix2dos > gtupload_manual.txt
endif

GeneTorrent.1: man_GeneTorrent.1
	cat $^ > $@
if CYGWIN
	MANWIDTH=80 man -P cat ./$@ | col -b | unix2dos > GeneTorrent_manual.txt
endif

