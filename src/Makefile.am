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
   gtBlockIO.h                   \
   loggingmask.h                 \
   stringTokenizer.h

torrentrasterbar_CXXFLAGS =  $(shell $(PKG_CONFIG) --cflags ../libtorrent/libtorrent-rasterbar.pc)
torrentrasterbar_LIBS = -L../libtorrent/src/.libs $(shell $(PKG_CONFIG) --libs-only-l ../libtorrent/libtorrent-rasterbar.pc)

# lib_LTLIBRARIES = libgenetorrent.la
#
# libgenetorrent_la_SOURCES = gtBase.cpp \
#                             gtUtils.cpp \
#                             gtLog.cpp \
#                             gtAlerts.cpp \
#                             geneTorrentUtils.cpp \
#                             stringTokenizer.cpp
#
# libgenetorrent_la_CXXFLAGS = $(torrentrasterbar_CXXFLAGS)
#
# libgenetorrent_la_LDFLAGS = -version-info 0:0:0 \
#                             $(torrentrasterbar_LIBS) \
#                             -R $(libdir)/GeneTorrent
#
# libgenetorrent_la_LIBADD = -lboost_system \
#                            -lboost_filesystem \
#                            -lboost_regex \
#                            -lboost_program_options

bin_PROGRAMS = GeneTorrent
#              GeneTorrent_Upload \
#              GeneTorrent_Download \
#              GeneTorrent_Server

GeneTorrent_Common_SOURCES = gtMain.cpp

GeneTorrent_SOURCES = $(GeneTorrent_Common_SOURCES) \
                      gtUpload.cpp                  \
                      gtDownload.cpp                \
                      gtServer.cpp                  \
                      gtBase.cpp                    \
                      gtUtils.cpp                   \
                      gtLog.cpp                     \
                      gtAlerts.cpp                  \
                      geneTorrentUtils.cpp          \
                      gtNullStorage.cpp             \
                      gtZeroStorage.cpp				\
                      stringTokenizer.cpp

# GeneTorrent_Upload_SOURCES = $(GeneTorrent_Common_SOURCES) \
#                              gtUpload.cpp
#
# GeneTorrent_Download_SOURCES = $(GeneTorrent_Common_SOURCES) \
#                                gtDownload.cpp
#
# GeneTorrent_Server_SOURCES = $(GeneTorrent_Common_SOURCES) \
#                              gtServer.cpp

GTdatadir = $(datadir)/GeneTorrent

dist_GTdata_DATA = dhparam.pem

GeneTorrent_CPPFLAGS = $(BOOST_CPPFLAGS) \
                       $(OPENSSL_INCLUDES)

GeneTorrent_CXXFLAGS = $(torrentrasterbar_CXXFLAGS) \
                       -I$(top_srcdir)/libtorrent/include \
                       -DGENETORRENT_ALL

GeneTorrent_LDFLAGS = $(torrentrasterbar_LIBS) \
                      -Wl,-rpath=$(libdir)/GeneTorrent \
                      $(BOOST_LDFLAGS) \
                      $(OPEENSSL_LDFLAGS)

GeneTorrent_LDADD =  -lboost_system \
                     -lboost_filesystem \
                     -lboost_regex \
                     -lboost_program_options \
                     -lssl \
                     -lcrypto
#                    libgenetorrent.la

# GeneTorrent_Upload_CXXPFLAGS = $(torrentrasterbar_CXXFLAGS) \
#                               -DGENETORRENT_UPLOAD
#
# GeneTorrent_Upload_LDFLAGS = $(GeneTorrent_LDFLAGS)
#
# GeneTorrent_Upload_LDADD = $(GeneTorrent_LDADD)
#
# GeneTorrent_Download_CXXFLAGS = $(torrentrasterbar_CXXFLAGS) \
#                                 -DGENETORRENT_DOWNLOAD
#
# GeneTorrent_Download_LDFLAGS = $(GeneTorrent_LDFLAGS)
#
# GeneTorrent_Download_LDADD = $(GeneTorrent_LDADD)
#
# GeneTorrent_Server_CXXFLAGS = $(torrentrasterbar_CXXFLAGS) \
#                               -DGENETORRNET_SERVER
#
# GeneTorrent_Server_LDFLAGS = $(GeneTorrent_LDFLAGS)
#
# GeneTorrent_Server_LDADD = $(GeneTorrent_LDADD)

dist_man_MANS = GeneTorrent.1

