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
depsDir=${baseDir}

OPENSSL_SRC_URL=http://www.openssl.org/source/openssl-1.0.1c.tar.gz
OPENSSL_VER=openssl-1.0.1c
OPENSSL_MD5=ae412727c8c15b67880aef7bd2999b2e

WGET="$(which wget) --tries 3"
CURL="$(which curl) -o ${OPENSSL_VER}.tar.gz"

function errexit
{
   echo "Error building openssl: $1"
   exit 1
}

if [ "$(which wget)" == "" -a "$(which curl)" == "" ]; then
   errexit "Need curl or wget to download openssl source."
fi

if [ ! -z "$(which wget)" ]; then
   DLTOOL=${WGET}
else
   DLTOOL=${CURL}
fi

mkdir -p "${depsDir}"
pushd "${depsDir}" > /dev/null

if [ -e ${OPENSSL_VER}.tar.gz ]; then
   echo "Checking MD5 hash of existing file..."
   FILE_MD5=$(md5sum ${OPENSSL_VER}.tar.gz | cut -f 1 -d" ")

   if [ "${FILE_MD5}" != "${OPENSSL_MD5}" ]; then
      errexit "file md5 sum is incorrect -  please delete it and run the script again"
   fi
else
   ${DLTOOL} ${OPENSSL_SRC_URL}

   if [ $? -ne 0 ]; then
      errexit "download operation failed"
   fi

   echo "Checking MD5 hash of downloaded file..."
   FILE_MD5=$(md5sum ${OPENSSL_VER}.tar.gz | cut -f 1 -d" ")

   if [ "${FILE_MD5}" != "${OPENSSL_MD5}" ]; then
      errexit "file md5 sum is incorrect -  please delete it and run the script again"
   fi
fi

tar xzvf ${OPENSSL_VER}.tar.gz
if [ $? -ne 0 ]; then
   errexit "untar operation failed"
fi

pushd ${OPENSSL_VER} > /dev/null

./config --openssldir=${depsDir} shared

if [ $? -ne 0 ]; then
   errexit "openssl bootstrap operation failed"
fi

make install

if [ $? -ne 0 ]; then
   errexit "openssl build operation failed"
fi

popd > /dev/null # pops from openssl tarball back to deps

popd > /dev/null # pops from deps to original working directory

echo "openssl build was successful"
exit 0

