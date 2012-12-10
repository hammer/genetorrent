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
AC_INIT([GeneTorrent], [3.7.5], [], [GeneTorrent])
AC_CONFIG_AUX_DIR([autotools])
AM_INIT_AUTOMAKE([1.11 foreign])
AC_CONFIG_SRCDIR([src/gtBase.cpp])
AC_CONFIG_HEADERS([src/gt_config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_SYS_LARGEFILE()
AC_CANONICAL_HOST

# Silencing build output (automake-1.11)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Set compiler flag defaults
: ${CFLAGS="-g -O2 -Wall"}
: ${CXXFLAGS="-g -O2 -Wall"}

AS_ECHO
AS_ECHO "Initializing libtool:"
LT_PREREQ([2.2.6])
LT_INIT()
LT_LANG([C++])

# Checks for programs.
AS_ECHO
AS_ECHO "Checking for C/C++ compilers:"
AC_PROG_CXX
AC_PROG_CXX_C_O
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP

AS_IF([test -z "$CXX"],
   [AC_MSG_ERROR([C++ compiler not found.])])

AS_ECHO
AS_ECHO "Checking for programs:"
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_LANG_CPLUSPLUS
PKG_PROG_PKG_CONFIG
AS_IF([test -z "$PKG_CONFIG"],
   [AC_MSG_ERROR([pkg-config not found.])])

AS_ECHO
AS_ECHO "Checking for Boost libraries >= 1.48.0:"
AX_BOOST_BASE([1.48.0])
AX_BOOST_REGEX()
AS_IF([test -z "$BOOST_REGEX_LIB"],
   [AC_MSG_ERROR([Boost regex library not found.])])
AX_BOOST_SYSTEM()
AS_IF([test -z "$BOOST_SYSTEM_LIB"],
   [AC_MSG_ERROR([Boost system library not found.])])
AX_BOOST_FILESYSTEM()
AS_IF([test -z "$BOOST_FILESYSTEM_LIB"],
   [AC_MSG_ERROR([Boost filesystem library not found.])])
AX_BOOST_PROGRAM_OPTIONS()
AS_IF([test -z "$BOOST_PROGRAM_OPTIONS_LIB"],
   [AC_MSG_ERROR([Boost program_options library not found.])])

AS_ECHO
AS_ECHO "Checking for Python >= 2.4:"
AM_PATH_PYTHON(2.4)

AS_ECHO
AS_ECHO "Checking for pthread support:"
AX_PTHREAD()

AS_ECHO
AS_ECHO "Checking for required libraries:"

AX_LIB_XERCES()
AS_IF([ test x"$HAVE_XERCES" != xyes ], [AC_MSG_ERROR([xerces library required but not found])], [])
AC_CHECK_LIB([curl], [curl_easy_init], [],
   [AC_MSG_ERROR([curl library required but not found])])
AX_CHECK_XQILLA([],
   [AC_MSG_ERROR([xqilla library required but not found. Try using --with-xqilla=DIR])
   ])
AX_CHECK_OPENSSL([],
   [AC_MSG_ERROR([openssl library required but not found. Try using --with-openssl=DIR])
   ])

#AC_CHECK_LIB([rt], [main])
#AC_CHECK_LIB([idn], [main])
#AC_CHECK_LIB([icule], [main])

# Checks for header files.
AS_ECHO
AS_ECHO "Checking for required headers:"
AC_CHECK_HEADERS([inttypes.h stdlib.h string.h sys/statvfs.h sys/time.h syslog.h unistd.h])
AC_CHECK_HEADER([curl/curl.h], [],
   [AC_MSG_ERROR([curl headers required but not found])])

AM_CONDITIONAL([CYGWIN],
    [case $host_os in *cygwin*) true;; *) false;; esac])

AM_CONDITIONAL([DARWIN],
    [case $host_os in *darwin*) true;; *) false;; esac])

AM_CONDITIONAL([LINUX],
    [case $host_os in *linux*) true;; *) false;; esac])

AM_CONDITIONAL([SOLARIS],
    [case $host_os in *solaris*) true;; *) false;; esac])

# Override pythondir on Mac OS X to install into correct
# site-packages directory
if ${DARWIN_TRUE}; then
    pythondir=/Library/Python/2.7/site-packages
fi

# Not necessary because AX_BOOST_*() performs the header check for us
# AC_CHECK_HEADER([boost/system.hpp])
# AC_CHECK_HEADER([boost/filesystem.hpp])
# AC_CHECK_HEADER([boost/regex.hpp])
# AC_CHECK_HEADER([boost/program_options.hpp])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([getcwd gettimeofday localtime_r mkdir pathconf])
AC_CHECK_FUNCS([strdup strerror strndup strrchr strtol strtoul])
AC_CHECK_FUNCS([getrlimit setrlimit])

AC_CONFIG_FILES(
   [Makefile]
   [src/Makefile]
   [GeneTorrent.spec]
   [GeneTorrent.nsi]
   [scripts/Makefile]
   [scripts/GTLoadBalancer]
   [scripts/python_wrapper]
   [scripts/gtoinfo.py]
   [scripts/gtocheck]
   [init.d/Makefile]
   [deps/boost/Makefile]
   [deps/openssl/Makefile]
   [tests/Makefile]
   [dpkg/Makefile]
   [deps/xqilla/Makefile]
   [deps/xerces-c/Makefile]
   [macosx/Distribution.xml]
)

AC_CONFIG_SUBDIRS(
   [libtorrent]
)

AC_OUTPUT

