#!/bin/bash
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

baseDir=$(pushd $(dirname $0) > /dev/null; pwd -P; popd > /dev/null)

DEP_NAME=xerces-c
SRC_URL=http://mirror.sdunix.com/apache//xerces/c/3/sources/xerces-c-3.1.1.tar.gz
DEP_VER=xerces-c-3.1.1
EXPECTED_MD5=6a8ec45d83c8cfb1584c5a5345cb51ae
CONFIG_CMD="./configure --prefix=${baseDir}"
if [ $(uname -s) = "Darwin" ] ; then
    # force use of standard libraries rather than those in macports, etc"
    CONFIG_CMD="${CONFIG_CMD} --with-curl=/usr --with-icu=/usr"
fi
BUILD_CMD="make install $@"
TARBALL="${DEP_VER}.tar.gz"

. ${baseDir}/../builder_common
