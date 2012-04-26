Copyright (c) 2011-2012, Annai Systems, Inc.  All rights reserved.
 
Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:
 
1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
  
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 
Created under contract by Cardinal Peak, LLC.   www.cardinalpeak.com

-------------------------------------------------------------------------------
This is the read me file for GeneTorrent

Directory Contents

build.sh            Master build script
GeneTorrent         GeneTorrent root directory
libtorrent          libtorrent root directory
ReadMe.txt          This file
rpmbuild            Files required to construct a GeneTorrent RPM
scripts             Utility Scripts-part of GeneTorrent not the build process

Build Requirements:

The following packages and minimum versions must be installed on the build 
machine prior to using build.sh.

Boost 1.48 
xerces-c 3.0.1
xqilla 2.2.3
libcurl 7.19.7

On centos 5.x

openssl 1.0.1a

If using the source release of GeneTorrent, Boost 1.48 must be built without 
icu support and static libraries must be provided.
