#  Copyright (c) 2011, Annai Systems, Inc.
#  All rights reserved.
#  
#  Redistribution and use in source and binary forms, with or without #  modification, are permitted provided that the following conditions are met: #  
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


function bailout
{
   echo
   echo "============================================================================="
   echo "top level build script failure detected in in function $1" 
   echo "============================================================================="
   echo
   exit 1;
}

function build_libtorrent
{
   ./autotool.sh || bailout $FUNCNAME
   ./configure --enable-debug --enable-logging=verbose --disable-geoip --disable-dht --prefix=/usr --with-boost-libdir=/usr/lib64 --libdir=/usr/lib64 CFLAGS="-g -O2" CXXFLAGS="-g -O2" || bailout $FUNCNAME
   make clean || bailout $FUNCNAME
   make -j 14 || bailout $FUNCNAME
   sudo make install || bailout $FUNCNAME
}

function build_GeneTorrent
{
   ./autogen.sh || bailout $FUNCNAME
   ./configure --prefix=/usr CFLAGS="-g -O2 -Wall" CXXFLAGS="-g -O2 -Wall" || bailout $FUNCNAME
   make clean || bailout $FUNCNAME
   make || bailout $FUNCNAME
   sudo make install || bailout $FUNCNAME
}

function build_scripts
{
   ./autogen.sh || bailout $FUNCNAME
   ./configure --prefix=/usr || bailout $FUNCNAME
   sudo make install || bailout $FUNCNAME
}

for dir in libtorrent GeneTorrent scripts
do
   saveDir=${PWD}
   cd ${dir}
   build_${dir}
   cd ${saveDir}
done
