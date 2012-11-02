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

XQILLA_SRC_URL=http://downloads.sourceforge.net/project/xqilla/xqilla/2.3.0/XQilla-2.3.0.tar.gz
XQILLA_VER=XQilla-2.3.0
XQILLA_MD5=7261c7b4bb5a45cbf6270073976a51ce

XQILLA_TARBALL="/tmp/${XQILLA_VER}.tar.gz"

WGET="$(which wget) --tries 3 -O ${XQILLA_TARBALL}"
CURL="$(which curl) -o ${XQILLA_TARBALL} -L"

function errexit
{
   echo "Error building boost: $1"
   exit 1
}

if [ "$(which wget)" == "" -a "$(which curl)" == "" ]; then
   errexit "Need curl or wget to download boost source."
fi

if [ ! -z "$(which wget)" ]; then
   DLTOOL="${WGET}"
else
   DLTOOL="${CURL}"
fi

pushd "${baseDir}" > /dev/null

if [ -e ${XQILLA_TARBALL} ]; then
   echo "Checking MD5 hash of existing file..."
   FILE_MD5=$(md5sum ${XQILLA_TARBALL} | cut -f 1 -d" ")

   if [ "${FILE_MD5}" != "${XQILLA_MD5}" ]; then
      errexit "file md5 sum is incorrect -  please delete it and run the script again"
   fi
else
   ${DLTOOL} ${XQILLA_SRC_URL}

   if [ $? -ne 0 ]; then
      errexit "download operation failed"
   fi

   echo "Checking MD5 hash of downloaded file..."
   FILE_MD5=$(md5sum ${XQILLA_TARBALL} | cut -f 1 -d" ")

   if [ "${FILE_MD5}" != "${XQILLA_MD5}" ]; then
      errexit "file md5 sum is incorrect -  please delete it and run the script again"
   fi
fi

tar xzf ${XQILLA_TARBALL}
if [ $? -ne 0 ]; then
   errexit "untar operation failed"
fi

pushd ${XQILLA_VER} > /dev/null

./configure --prefix=${baseDir} --with-xerces=/usr

if [ $? -ne 0 ]; then
   errexit "xqilla configure operation failed"
fi

make install $1

if [ $? -ne 0 ]; then
   errexit "xqilla build operation failed"
fi

popd > /dev/null # pops from xqilla tarball back to deps

popd > /dev/null # pops from deps to original working directory

echo "xqilla build was successful"
exit 0

