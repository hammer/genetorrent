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

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])

# make the script version number dependent upon the version number of
# GeneTorrent.  This is an ugly bit of m4 programming that grabs the
# version number out of the file ../GeneTorrent/configure.ac.  The
# m4_bpatsubst nonsense is there to strip that trailing newline left
# there by esyscmd

m4_define(scriptver, 
          m4_bpatsubst(esyscmd([sed -ne '/^AC_INIT/ s/^AC_INIT(.*, \([^)]*\))$/\1/p' ../GeneTorrent/configure.ac]), [\([0-9.]*\)\(\w\|\W\)*], [\1]))

AC_INIT(GeneTorrentScripts, scriptver)

AM_MAINTAINER_MODE
AM_INIT_AUTOMAKE

AM_PATH_PYTHON([2.4])

AC_CONFIG_FILES([
Makefile 
GTLoadBalancer
python_wrapper
gtoinfo.py
gtocheck
])
AC_OUTPUT

