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

DEP_NAME=boost
SRC_URL=http://downloads.sourceforge.net/project/boost/boost/1.48.0/boost_1_48_0.tar.gz
DEP_VER=boost_1_48_0
EXPECTED_MD5=313a11e97eb56eb7efd18325354631be
CONFIG_CMD="./bootstrap.sh --prefix=${baseDir} --with-libraries=system,regex,filesystem,program_options,thread --without-icu"
BUILD_CMD="./b2 link=shared warnings=all debug-symbols=on install $@"
TARBALL="/tmp/${DEP_VER}.tar.gz"

. ${baseDir}/../builder_common
