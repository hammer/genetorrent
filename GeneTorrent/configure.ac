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
AC_INIT(GeneTorrent, 1.0.0.5)
AM_MAINTAINER_MODE
AC_CONFIG_AUX_DIR([autotools])
AM_INIT_AUTOMAKE
PKG_PROG_PKG_CONFIG
AC_CONFIG_SRCDIR([src/tclapOutput.cpp])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_LANG_CPLUSPLUS

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lboost_regex':
# static linking AC_CHECK_LIB([boost_regex], [main])
# FIXME: Replace `main' with a function in `-lboost_system':
# static linking AC_CHECK_LIB([boost_system], [main])
# FIXME: Replace `main' with a function in `-lboost_system':
# static linking AC_CHECK_LIB([boost_filesystem], [main])
# FIXME: Replace `main' with a function in `-lcurl':
AC_CHECK_LIB([curl], [main])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])
# FIXME: Replace `main' with a function in `-lxqilla':
AC_CHECK_LIB([xqilla], [main])
# FIXME: Replace `main' with a function in `-lxqilla':
AC_CHECK_LIB([log4cpp], [main])

AC_CHECK_LIB([c], [main])
AC_CHECK_LIB([rt], [main])
AC_CHECK_LIB([ssl], [main])
AC_CHECK_LIB([ssl3], [main])
AC_CHECK_LIB([ssh2], [main])
AC_CHECK_LIB([ldap], [main])
AC_CHECK_LIB([idn], [main])
AC_CHECK_LIB([icule], [main])

PKG_CHECK_MODULES(torrentrasterbar, libtorrent-rasterbar)
 
# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h curl/curl.h])
AC_CHECK_HEADERS(sstream) # needed for tclap

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_CHECK_FUNCS([getcwd mkdir pathconf strtol])

AC_CONFIG_FILES([
Makefile 
src/Makefile 
])
AC_OUTPUT

